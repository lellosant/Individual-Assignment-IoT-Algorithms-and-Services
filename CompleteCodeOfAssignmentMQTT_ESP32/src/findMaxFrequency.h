#include <arduinoFFT.h>
#include <Arduino.h>
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "parameter.h"
#include <math.h>



float maxFrequency = 0;

uint32_t sample_rate = MAX_SAMPLE_RATE;
#define PRECISION 2 //1Hz of precision (sample_rate/SAMPLES)<2





float  getMaxFrequencyformFFT(float* fillReal, float* fillImag, uint32_t samples, u_int32_t sample_rate){
    uint64_t start = micros();
    ArduinoFFT<float> FFT = ArduinoFFT<float>(fillReal, fillImag, samples, sample_rate);
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    uint64_t inizialization_time = micros() - start;
    start = micros();
    FFT.compute(FFT_FORWARD);
    FFT.complexToMagnitude();
    uint64_t fft_calc_time =  micros() - start;
    int topBin = 0;
    
    float maxVal = 0;

    //avg of values
    float avg = 0;
    for (int i = 0; i < SAMPLES / 2; i++) {
      avg += fillReal[i];
      if (fillReal[i] > maxVal) { maxVal = fillReal[i]; }
    }
    avg = avg / ((SAMPLES/2) - 1);


    //variance calc
    float variance = 0;
    //start for 1 because bin 0 is DC component
    for (int i = 1; i < SAMPLES/2; i++)
      variance += (fillReal[i] - avg) * (fillReal[i] - avg);

    variance /= ((SAMPLES/2) - 1);
    float sigma = sqrtf(variance);

    float threshold = avg + SNR_MULTIPLIER * sigma;
      //float threshold = avg *(SAMPLES / 32); // Scaled: Peak * (SAMPLES/something)
      for (int i = (SAMPLES / 2) - 1; i > 0; i--) {
        if (fillReal[i] > threshold) {
          topBin = i;
          break; 
        }
      }
      float f_max = (topBin * sample_rate) / (float)SAMPLES;
      Serial.printf("FFT compute on %d SAMPLES\nInizialization Time: %d us\nFFT Calculation Time: %d us\nTotal time: %d us\n",SAMPLES, (int)inizialization_time, (int)fft_calc_time, (int)(inizialization_time+fft_calc_time));
      Serial.printf("Detected Max Freq: %.2f Hz in bin %d\n New Suggested sampling frequency: %.2f Hz\nSAMPLE FREQUENCY=%d\n\n", f_max, topBin, f_max * 2.5, sample_rate);
      return f_max;

}


float findMaxFrequency() {
  uint32_t bytes_read;
  uint16_t dma_buffer[BUFFER_DMA_SIZE];
  float last_freq=0;
  float fillReal[SAMPLES], fillImag[SAMPLES];
  //start sampling with dma at max sample frequency
  i2s_set_sample_rates(I2S_NUM, sample_rate);

  while (maxFrequency == 0) { 


      
      int sample_index=0;
      int delta; 
      if (sample_rate<20000)
        //virtual sampling with DMA (DMA set to 20KHz)
        delta = 20000/sample_rate;
      else 
        //real sampling with DMA
        delta = 1;

      
      i2s_adc_enable(I2S_NUM);
      while (sample_index < SAMPLES){

        esp_err_t result = i2s_read(I2S_NUM, &dma_buffer, sizeof(dma_buffer),&bytes_read, portMAX_DELAY); 

        if (result != ESP_OK || bytes_read == 0) 
          continue;

        int samples_received = bytes_read / sizeof(uint16_t); 
        
        for (int i = 0; i < samples_received; i+=delta) {
          fillReal[sample_index] = (float)(dma_buffer[i] & 0x0FFF) - 2048.0; //-2048 for centering around 0 the FFT
          fillImag[sample_index] = 0.0;
          sample_index++;
          
          if(sample_index>=SAMPLES)break;
        }
      }
      i2s_adc_disable(I2S_NUM);
    
    uint32_t last_sample_rate = sample_rate;  
    //FFT
    float freq_detected = getMaxFrequencyformFFT(fillReal, fillImag, SAMPLES, sample_rate);
    
    if((sample_rate/SAMPLES)<=PRECISION || last_freq == freq_detected || last_sample_rate == MIN_SAMPLE_RATE){
      // i found the max frequency
      maxFrequency = freq_detected;
      break;
    }

    
    sample_rate = (uint32_t)max((float)(freq_detected * 2.5f), (float)MIN_SAMPLE_RATE); //Shannon Nyquest Theorem

  
    //i have just do a cycle with same frequency detected 
    // or last frequency is 1000Hz

   /* if(last_freq == freq_detected || last_sample_rate == MIN_SAMPLE_RATE){
      


    };
*/
    last_freq = freq_detected;



    if(sample_rate>=20000){
      i2s_set_sample_rates(I2S_NUM, sample_rate);
      continue;
    }

    //sample rate is < 20000Hz


  
    if(sample_rate > MIN_SAMPLE_RATE){
      //adapting the sampling rate to a submultiple of 20000 Hz
      sample_rate = 20000/(int)(ceil(20000/sample_rate));
      i2s_set_sample_rates(I2S_NUM, 20000);
      continue;
    }

    //sample rate is under MIN_SAMPLE_RATE
    sample_rate = 20000/(int)(ceil(20000/MIN_SAMPLE_RATE));
    i2s_set_sample_rates(I2S_NUM, 20000);
      
}


Serial.printf("I found The max frequency of this signal that is %.2f  Hz", maxFrequency);
return maxFrequency;

}


