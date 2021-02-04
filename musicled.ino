// Arduino Beat Detector By Damian Peckett 2015
// License: Public Domain.

// Our Global Sample Rate, 5000hz
#define SAMPLEPERIODUS 200

#define INPUT_PIN 0
#define LED_RED 9
#define LED_GREEN 10
#define LED_BLUE 11

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void setup() {
    // Set ADC to 77khz, max for 10bit
    sbi(ADCSRA,ADPS2);
    cbi(ADCSRA,ADPS1);
    cbi(ADCSRA,ADPS0);

    //The pin with the LED
    pinMode(9, OUTPUT);
    pinMode(10, OUTPUT);
    pinMode(11, OUTPUT);
    digitalWrite(10, LOW);
    digitalWrite(11, LOW);
}

// 20 - 200hz band pass
float bassFilter(float sample) {
    static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
    xv[0] = xv[1]; xv[1] = xv[2]; 
    xv[2] = (sample) / 2.f; // change here to values close to 2, to adapt for stronger or weaker sources of line level audio  
    

    yv[0] = yv[1]; yv[1] = yv[2]; 
    yv[2] = (xv[2] - xv[0])
        + (-0.7960060012f * yv[0]) + (1.7903124146f * yv[1]);
    return yv[2];
}

float midFilter(float x) {//class II 
  static float v[2] = {0,0};
  v[0] = v[1];
  v[1] = (6.452634283659581804e-1 * x)
     + (0.29052685673191641635 * v[0]);
  return 
     (v[1] - v[0]);
}

// 10hz Single Pole Lowpass IIR Filter
float envelopeFilterLow(float sample) { //10hz low pass
    static float xv[2] = {0,0}, yv[2] = {0,0};
    xv[0] = xv[1]; 
    xv[1] = sample / 50.f;
    yv[0] = yv[1]; 
    yv[1] = (xv[0] + xv[1]) + (0.9875119299f * yv[0]);
    return yv[1];
}

// 1.7 - 3.0hz Single Pole Bandpass IIR Filter
float beatFilter(float sample) {
    static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
    xv[0] = xv[1]; xv[1] = xv[2]; 
    xv[2] = sample / 2.7f;
    yv[0] = yv[1]; yv[1] = yv[2]; 
    yv[2] = (xv[2] - xv[0])
        + (-0.7169861741f * yv[0]) + (1.4453653501f * yv[1]);
    return yv[2];
}

void loop() {
    unsigned long time = micros(); // Used to track rate
    float sample, valueLow, valueMid, envelopeLow, envelopeMid, beatLow, beatMid, threshLow, threshMid;
    unsigned char i;
    
    threshLow = 0.02f * 600;
    threshMid = 0.02f * 10;

    float currentBrightness = 0.0f;
    float currentColor = 0.0f;
    float colorCycle = 0.0f;
    float red, green, blue;
    long lastDetection = 0;
    long currentDetection = 0;

    for(i = 0;;++i){
        if (millis() - lastDetection > 60) {
          if (currentBrightness > 0.45)
            currentBrightness = currentBrightness * 0.997;
          else
            currentBrightness = max(0.1f, currentBrightness*0.998f);
        }
        analogWrite(LED_RED, 255 * red * currentBrightness);
        analogWrite(LED_GREEN, 255 * green * currentBrightness);
        analogWrite(LED_BLUE, 255 * blue * currentBrightness);
        
        
        // Read ADC and center so +-512
        sample = (float)analogRead(INPUT_PIN)-512.f;

        // Filter band passes
        valueLow = bassFilter(sample);
        valueMid = midFilter(sample);

        // Take signal amplitude and filter
        if(valueLow < 0)valueLow=-valueLow;
        envelopeLow = envelopeFilterLow(valueLow);
        if(valueMid < 0)valueMid=-valueMid;
        envelopeMid = envelopeFilterLow(valueMid);

        // Every 200 samples (25hz) filter the envelope 
        if(i == 200) {
                // Filter out repeating bass sounds 60 - 300bpm
                beatLow = beatFilter(envelopeLow);
                beatMid = beatFilter(envelopeMid);

                // If we are above threshold, light up LED
                if(beatLow > threshLow) {
                  currentBrightness = 1.0f;
                  lastDetection = millis();
                }
                if (beatMid > threshMid) colorCycle = colorCycle + 0.017137f;
                if (colorCycle >= 100.0f) colorCycle = 0.0f;
                currentColor = 3.0f * (colorCycle - floor(colorCycle));
                red = 1.0f - currentColor;
                if (currentColor < 1.0f) {
                  red = 1.0f - currentColor;
                  green = currentColor;
                  blue = 0.0f;
                }
                else if (currentColor < 2.0f) {
                  red = 0.0f;
                  green = 2.0f - currentColor;
                  blue = currentColor - 1.0f;
                }
                else if (currentColor < 3.0f) {
                  red = currentColor - 2.0f;
                  green = 0.0f;
                  blue = 3.0f - currentColor;
                }

                //Reset sample counter
                i = 0;
        }

        // Consume excess clock cycles, to keep at 5000 hz
        for(unsigned long up = time+SAMPLEPERIODUS; time > 20 && time < up; time = micros());
    }  
}
