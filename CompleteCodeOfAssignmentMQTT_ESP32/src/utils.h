#include <Arduino.h>
#include "driver/i2s.h"


void initDMAi2s(uint32_t sample_rate, int buffer_size, int buffer_count, i2s_port_t i2s_num, adc_unit_t adc_unit, adc1_channel_t adc_channel) {

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
  //i2s_adc_enable(i2s_num);
}

