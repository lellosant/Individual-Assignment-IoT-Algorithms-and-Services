#include <Arduino.h>
#include <math.h>

#define SAMPLE_RATE     100
#define I2S_NUM         I2S_NUM_0
#define BUFFER_SIZE     64

#define FREQUECY1 2
#define FREQUECY2 5
float p = 0.02;             // Anomaly probability (2%)
float sigma = 0.2;          // Gaussian noise standard deviation


#define SINE_TABLE_SIZE 8192
float sineTable[SINE_TABLE_SIZE];

float phaseIndex1 = 0;
float phaseIndex2 = 0;

float deltaPhase1 = (FREQUECY1 * SINE_TABLE_SIZE) / SAMPLE_RATE; 
float deltaPhase2 = (FREQUECY2 * SINE_TABLE_SIZE) / SAMPLE_RATE; 

void setup() {


  Serial.begin(115200);
  
  //precompute sine values
  for (int i = 0; i < SINE_TABLE_SIZE; i++) {
    sineTable[i] = sinf(2.0 * M_PI * i / SINE_TABLE_SIZE);
  }

}


float t = 0.0; 
void loop() {
  

  
  size_t bytes_written;


  float val;
  uint8_t dac_val;
  for (int i = 0; i < BUFFER_SIZE; i++) {


  //clean signal
  float clean_val = 2.0 * sineTable[(int)phaseIndex1] + 4.0 * sineTable[(int)phaseIndex2];

  //Gaussian Noise n(t) using Box-Muller Transform
  float u1 = (float)random(1, 10001) / 10000.0;
  float u2 = (float)random(1, 10001) / 10000.0;
  float noise = sigma * sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);

  //Anomaly Injection A(t)
  float anomaly = 0;
  if ((float)random(0, 1000) / 1000.0 < p) {
      // Random magnitude between 5 and 15, random sign
      float magnitude = 5.0 + ((float)random(0, 1000) / 100.0); 
      anomaly = (random(0, 2) == 0) ? magnitude : -magnitude;
  }


    //i want generate signal of this form 2*sin(2*pi*3*t)+4*sin(2*pi*5*t) + n(t) + A(t)
    float val = clean_val + noise + anomaly;

    phaseIndex1 += deltaPhase1;
    phaseIndex2 += deltaPhase2;


    //normalization for the DAC (0 to 255)
    dac_val = (uint8_t)((val / 6.0) * 127.0 + 128.0);

    dacWrite(25, dac_val);

    Serial.printf("%d\t\n", dac_val);

    

    if (phaseIndex1 >= SINE_TABLE_SIZE) {
      phaseIndex1 -= SINE_TABLE_SIZE;
    }
    if (phaseIndex2 >= SINE_TABLE_SIZE) {
      phaseIndex2 -= SINE_TABLE_SIZE;
    }

    vTaskDelay(pdMS_TO_TICKS(1000/SAMPLE_RATE)); 
  
  }

}