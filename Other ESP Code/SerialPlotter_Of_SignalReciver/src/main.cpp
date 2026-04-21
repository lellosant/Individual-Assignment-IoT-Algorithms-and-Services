#include <Arduino.h>
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SAMPLE_RATE 100
#define I2S_NUM     I2S_NUM_0
#define BUFFER_SIZE 64


void printBetterSerialPlotter(void *parameter) {
  while (1) {
    uint16_t buffer[BUFFER_SIZE];
    size_t bytes_read;

    //read samples from ADC using I2S
    i2s_read(I2S_NUM, &buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

    if (bytes_read > 0) {
      //clean the 4 unused bits (bit 15-12) as they are not used in ADC readings
      int adc_val = buffer[0] & 0x0FFF;

      //print the ADC value to serial in a format suitable for Serial Plotter
      
      Serial.printf("%d\t\n", adc_val);
    }

    //delay to allow Serial Plotter to process the data
    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

void setup() {
  Serial.begin(115200);

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SIZE * sizeof(uint16_t),
    .use_apll = false
  };

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6); // GPIO34
  i2s_adc_enable(I2S_NUM);


  //task to print samples to better serial plotter
  xTaskCreate(printBetterSerialPlotter, "SerialPlotter", 2048, NULL, 1, NULL);

}

void loop() {
}