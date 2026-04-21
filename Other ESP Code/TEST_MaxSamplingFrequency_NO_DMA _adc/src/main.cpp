#include <Arduino.h>
#include "driver/adc.h"

#define PIN_ADC     ADC1_CHANNEL_6  
#define SAMPLE_SIZE 100000


unsigned long time_start = 0;
unsigned long time_end = 0;

void setup() {

  int cpu_freq = getCpuFrequencyMhz();

  Serial.begin(115200);
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(PIN_ADC, ADC_ATTEN_DB_12);

  
}


uint_fast32_t cpu_freq[] = { 80, 160, 240}; // Available CPU frequencies in MHz

int i = 0;
void loop() {


  setCpuFrequencyMhz(cpu_freq[i]); // Set CPU frequency to 240 MHz for maximum performance
  i = (i + 1) % (sizeof(cpu_freq) / sizeof(cpu_freq[0])); 
  


 

  
  Serial.printf("Starting ADC sampling frequency test... CPU Frequency: %d MHz\n", getCpuFrequencyMhz());

  time_start = micros();

  for (int i = 0; i < SAMPLE_SIZE; i++) {
    volatile int adc_value = adc1_get_raw(PIN_ADC);
  }

  time_end = micros();

  unsigned long elapsed_time = time_end - time_start;
  
  Serial.printf("Time taken to read %d samples: %lu milliseconds\n", SAMPLE_SIZE, elapsed_time/1000);

  Serial.printf("Average time per sample: %.2f microseconds\n", (float)elapsed_time / SAMPLE_SIZE);

  Serial.printf("Average samples frequency: %.2f Hz\n", (float)SAMPLE_SIZE / (elapsed_time / 1000000.0));

  delay(10000); //10 seconds delay before next measurement

}