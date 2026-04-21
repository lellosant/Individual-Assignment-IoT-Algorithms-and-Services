#include <Arduino.h>
#include <math.h>

#define SAMPLE_RATE     1000
#define I2S_NUM         I2S_NUM_0
#define BUFFER_SIZE     64

#define FREQUECY1 3
#define FREQUECY2 5


#define SINE_TABLE_SIZE 8192
float sineTable[SINE_TABLE_SIZE];

float phaseIndex1 = 0;
float phaseIndex2 = 0;

float deltaPhase1 = (FREQUECY1 * SINE_TABLE_SIZE) / SAMPLE_RATE; //3Hz signal
float deltaPhase2 = (FREQUECY2 * SINE_TABLE_SIZE) / SAMPLE_RATE; //5Hz signal

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
  TickType_t xLastWakeTime;
  double period = (double)1000 /(double)SAMPLE_RATE;
  TickType_t xFrequency = 0 ;
  if(period>=1) xFrequency = pdMS_TO_TICKS(1000 / SAMPLE_RATE);
  for (int i = 0; i < BUFFER_SIZE; i++) {
    xLastWakeTime = xTaskGetTickCount();

    //i want generate signal of this form 2*sin(2*pi*3*t)+4*sin(2*pi*5*t)
    val =  2 * sineTable[(int)phaseIndex1] + 4 * sineTable[(int)phaseIndex2];

    phaseIndex1 += deltaPhase1;
    phaseIndex2 += deltaPhase2;


    //normalization for the DAC (0 to 255)
    dac_val = (uint8_t)((val / 6.0) * 127.0 + 128.0);

    dacWrite(25, dac_val);

    //Serial.printf("%d\t\n", dac_val);

    

    if (phaseIndex1 >= SINE_TABLE_SIZE) {
      phaseIndex1 -= SINE_TABLE_SIZE;
    }
    if (phaseIndex2 >= SINE_TABLE_SIZE) {
      phaseIndex2 -= SINE_TABLE_SIZE;
    }

    //if(1000/SAMPLE_RATE>0)vTaskDelay(pdMS_TO_TICKS(1000/SAMPLE_RATE)); 
    
  
    if(xFrequency>0)xTaskDelayUntil(&xLastWakeTime, xFrequency);
    else delayMicroseconds((double)1000000/(double)SAMPLE_RATE);
  }

}