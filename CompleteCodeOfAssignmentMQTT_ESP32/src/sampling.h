void setupDMA(uint32_t sample_freq){
    if(sample_freq>=20000)
        //set cpu to max frequency 
        //setCpuFrequencyMhz(F_CPU/1000000U);
        i2s_set_sample_rates(I2S_NUM, sample_freq);
    else
        i2s_set_sample_rates(I2S_NUM, 20000);

    i2s_adc_enable(I2S_NUM);
}

void setupADC(){
    i2s_adc_disable(I2S_NUM);
    i2s_driver_uninstall(I2S_NUM);
    
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_12);
}


float samplingAvgOnWindowDMA(uint32_t sample_freq, uint32_t samples,uint16_t* dmaBuffer, uint16_t bufferSize){
        uint64_t avg = 0;
        uint32_t n = 0; 
        //Use DMA for sampling
        uint32_t bytes_read;

        int delta;
        if(sample_freq < 20000)
            delta = 20000/sample_freq;
        else 
            delta = 1;
        
        while (n<samples){
            esp_err_t result = i2s_read(I2S_NUM, dmaBuffer, bufferSize ,&bytes_read, portMAX_DELAY);
            if (result != ESP_OK || bytes_read == 0) continue;
            int samples_received = bytes_read / 2; //2 is sizeof(uint16_t)
            for (int i = 0; i < samples_received; i+=delta) {
            avg+= (dmaBuffer[i] & 0x0FFF);
            n++;            
            if(n>=samples)break;
            }
        }
    return (float)avg / (float)n;
}



float samplingAvgOnWindowADC(uint32_t sample_freq, uint32_t samples){
    uint64_t avg = 0;
    uint32_t n = 0; 

    const TickType_t xFrequency = pdMS_TO_TICKS(1000 / sample_freq);
    //uint64_t us = (1000000/sample_freq) - 200; //200 is the approximate latency of wakeUp of light sleep

    while (n<samples){
        TickType_t xLastWakeTime = xTaskGetTickCount();
        avg += adc1_get_raw(ADC1_CHANNEL_6); 
        n++;


        //1 way is use task delay until, if no other task is running, the Power Manager automatically enters light sleep 
        xTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        //2 way is force light sleep, cause issue of wifi connection e MQTT connection
        //esp_sleep_enable_timer_wakeup(us);
        //esp_light_sleep_start(); 

        
    }
     return (float)avg / (float)n;
}


