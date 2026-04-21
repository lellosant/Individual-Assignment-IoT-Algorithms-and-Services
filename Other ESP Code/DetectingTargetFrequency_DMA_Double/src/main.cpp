#include <arduinoFFT.h>
#include <Arduino.h>
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

//note DMA minimum sample rate is 20 kHz, i do some test and with 10Khz the relevated frequency is Half and a 5Khz is a quarter
#define MAX_SAMPLE_RATE 2000000 // Hz 
#define SAMPLES 1024 // for FFT (must be a power of 2)
#define BUFFER_DMA 512


uint32_t sample_rate = MAX_SAMPLE_RATE;

#define I2S_NUM     I2S_NUM_0
#define ADC_UNIT    ADC_UNIT_1
#define ADC_CHANNEL ADC1_CHANNEL_6





uint32_t maxFrequency = 0;



void inizializeDMAi2s(uint32_t sample_rate, int buffer_size, int buffer_count, i2s_port_t i2s_num, adc_unit_t adc_unit, adc1_channel_t adc_channel) {

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = sample_rate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S, 
    .intr_alloc_flags = 0,
    .dma_buf_count = buffer_count,
    .dma_buf_len = buffer_size,
    .use_apll = false
  };

  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
  i2s_set_adc_mode(adc_unit, adc_channel); // GPIO34
  i2s_adc_enable(i2s_num);
}




double  getMaxFrequencyformFFT(double* fillReal, double* fillImag, uint32_t samples, u_int32_t sample_rate){
    ArduinoFFT<double> FFT = FFT = ArduinoFFT<double>(fillReal, fillImag, samples, sample_rate);

    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFT_FORWARD);
    FFT.complexToMagnitude();

    int topBin = 0;
      double avg = 0;
      double maxVal = 0;
      for (int i = 0; i < SAMPLES / 2; i++) {
        avg += fillReal[i];
        if (fillReal[i] > maxVal) { maxVal = fillReal[i]; }
      }

      avg = avg / (SAMPLES / 2 - 1);
      double threshold = avg *(SAMPLES / 32); // Scaled: Peak * (SAMPLES/something)
      for (int i = (SAMPLES / 2) - 1; i > 0; i--) {
        if (fillReal[i] > threshold) {
          topBin = i;
          break; 
        }
      }
      avg /= (SAMPLES / 2 - 1);

      double f_max = (topBin * sample_rate) / (double)SAMPLES;
      Serial.printf("Detected Max Freq: %.2f Hz | New Suggested Fs: %.2f Hz\nSAMPLE FREQUENCY=%d\n\n", f_max, f_max * 2.5, sample_rate);
      return f_max;

}


double medianValue(double *data, int size) {
    double temp[size];
    memcpy(temp, data, size * sizeof(double));
    std::sort(temp, temp + size);
    if (size % 2 == 0) {
        return (temp[size / 2 - 1] + temp[size / 2]) / 2.0;
    } else {
        return temp[size / 2];
    }
}


void TaskFindMaxFrequency(void *pvParameters) {
  uint32_t bytes_read;
  uint16_t dma_buffer[SAMPLES];
  uint16_t last_freq=0;
  double fillReal[SAMPLES], fillImag[SAMPLES];
  //start sampling with dma at max sample frequency
  i2s_set_sample_rates(I2S_NUM, sample_rate);

  while (maxFrequency == 0) { 

    if(sample_rate>20000){
      esp_err_t result = i2s_read(I2S_NUM, &dma_buffer, sizeof(dma_buffer),&bytes_read, portMAX_DELAY);    Serial.printf("letti %d byte\n", bytes_read);
      if (result != ESP_OK || bytes_read == 0) continue;
      int samples_received = bytes_read / 2; //2 is sizeof(uint16_t)
      for (int i = 0; i < samples_received; i++) {
      
        fillReal[i] = (double)(dma_buffer[i] & 0x0FFF) - 2048.0; //-2048 for centering around 0 the FFT
        fillImag[i] = 0.0;
        
      }

    }else{

      int sample_index=0;
      int delta = 20000/sample_rate;
      while (sample_index < SAMPLES){
        esp_err_t result = i2s_read(I2S_NUM, &dma_buffer, sizeof(dma_buffer),&bytes_read, portMAX_DELAY);    
        if (result != ESP_OK || bytes_read == 0) continue;
        int samples_received = bytes_read / 2; 
        for (int i = 0; i < samples_received; i+=delta) {
          fillReal[sample_index] = (double)(dma_buffer[i] & 0x0FFF) - 2048.0; //-2048 for centering around 0 the FFT
          fillImag[sample_index] = 0.0;
          sample_index++;
          
          if(sample_index>=SAMPLES)break;
        }
      }
    }


    double freq_detected = getMaxFrequencyformFFT(fillReal, fillImag, SAMPLES, sample_rate);

    uint32_t old_sample_rate = sample_rate;
    sample_rate = freq_detected * 2.5;//Shannon Nyquist theorem

  

    if(last_freq == (int)freq_detected){
      // i found the max frequency
      maxFrequency = freq_detected;
      Serial.printf("I found The max frequency of this signal that is %.2f  Hz", freq_detected);
      vTaskDelete(NULL);

    };

    last_freq = freq_detected;



    if(sample_rate>=20000){
      i2s_set_sample_rates(I2S_NUM, sample_rate);
    
      continue;
    }

    if(sample_rate<20000){


      if(old_sample_rate == 1000){
        // i found the max frequency
        maxFrequency = freq_detected;
        Serial.printf("I found The max frequency of this signal that is %.2f Hz", freq_detected);
        vTaskDelete(NULL);
      }
      if(sample_rate < 1000){
        sample_rate = 1000;
        i2s_set_sample_rates(I2S_NUM, 20000);
      }else{
              //adapting the sampling rate to a submultiple of 20000 Hz
      sample_rate = 20000/(int)(ceil(20000/sample_rate));
      i2s_set_sample_rates(I2S_NUM, 20000);
      }

      
      continue;
    }
}

//When Frequency is found
vTaskDelete(NULL);
}




    

void setup() {
  Serial.begin(115200);

  
  inizializeDMAi2s(sample_rate, BUFFER_DMA, 16, I2S_NUM, ADC_UNIT, ADC_CHANNEL);

  //core 1 for sampling (I/O bound), core 0 for FFT (CPU bound)
  xTaskCreatePinnedToCore(TaskFindMaxFrequency, "Sampler", 32000, NULL, 5, NULL, 1);
}

void loop() { vTaskDelete(NULL); }