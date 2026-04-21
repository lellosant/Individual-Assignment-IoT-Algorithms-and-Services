#include <Arduino.h>

void sleepTask(void *pvParameters){
  uint64_t time;
  while(1){
    
    //lightSleep
    esp_sleep_enable_timer_wakeup(10000);//10ms 
    time = micros();
    esp_light_sleep_start();
    uint64_t sleepTime = micros() - time;
    Serial.printf("Time for light sleep + wakeup latency: %llu us\n", sleepTime);
    Serial.printf("Time for light sleep: %d us\n", sleepTime-10000);//10 ms
    


  }
}

void setup() {
  Serial.begin(115200);
  xTaskCreatePinnedToCore(sleepTask,"sleepTask",10000,NULL,1,NULL,0);
}

void loop() {
  vTaskDelete(NULL);
}

