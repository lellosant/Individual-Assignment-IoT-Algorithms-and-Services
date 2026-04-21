#include <arduinoFFT.h>
#include <Arduino.h>
#include <WiFi.h>
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "utils.h"
#include "parameter.h"
#include "findMaxFrequency.h"
#include "secrets.h"
#include "sampling.h"
#include <PubSubClient.h>
#include "freertos/queue.h"
#include "esp_pm.h"


#define SAVE_ENERGY true



//SemaphoreHandle_t xSemaphoreFindMaxFrequency;
QueueHandle_t xAvgQueue;



void TaskFindMaxFrequency(void *pvParameters);
void TaskSampling(void *pvParameters);
void TaskWifi(void *pvParameters);
void TaskClientMQTT(void *pvParameters);



uint64_t startWindowTime;
uint64_t endWindowTime;

void setup() {
  startWindowTime = micros();
  Serial.begin(115200);

  //Power Management Configuration
  esp_pm_config_esp32_t pm_config = {
    .max_freq_mhz = 240, 
    .min_freq_mhz = 80, 
    .light_sleep_enable = true // Enter light sleep when no locks are taken
  };
  
  esp_err_t err = esp_pm_configure(&pm_config);
  if (err == ESP_OK) {
    Serial.println("Automatic Light Sleep On");
  }
  
  initDMAi2s(sample_rate, BUFFER_DMA_SIZE, NR_BUFFER_DMA, I2S_NUM, ADC_UNIT, ADC_CHANNEL);
  //xSemaphoreFindMaxFrequency = xSemaphoreCreateBinary();

  //create a queue that contain max 5 float value for MQTT task
  xAvgQueue = xQueueCreate(5, sizeof(float));
  
  xTaskCreatePinnedToCore(TaskFindMaxFrequency, "MaxFrequency", 32000, NULL, 5, NULL, 1);
  //xTaskCreatePinnedToCore(TaskSampling, "TaskSampling", 8192, NULL, 5, NULL, 1);
  

}

void loop() { vTaskDelete(NULL); }


//########################################
//           TaskFindMaxFrequency
//#########################################
void TaskFindMaxFrequency(void *pvParameters){
  
  //maxFrequency = findMaxFrequency();
  //xSemaphoreGive(xSemaphoreFindMaxFrequency);>
  
  //start to sampling and wifi and MQTT connection
  xTaskCreatePinnedToCore(TaskSampling, "TaskSampling", 8192, NULL, 5, NULL, 1);
  //xTaskCreatePinnedToCore(TaskWifi, "TaskWifi",4096,NULL,3,NULL, 0 ); //core 0 (wifi work on this core)
  //xTaskCreatePinnedToCore(TaskClientMQTT,"taskClientMQTT",4096,NULL,1,NULL,1);
  vTaskDelete(NULL);

}


//########################################
//           TaskSampling
//#########################################
void TaskSampling(void *pvParameters){


    //uint32_t sampleFreq = 1800000;
    uint32_t sampleFreq = 13;
    int nrSamples= ceil(sampleFreq*WINDOW_TIME_SAMPLING_SECONDS);

    //Sampling with ADC using CPU
    if(sampleFreq< 1000){
      Serial.printf("Continue sampling with ADC a the frequency of %d",sampleFreq);
      setupADC();
      while (1){
        float avg = samplingAvgOnWindowADC(sampleFreq, nrSamples);
        Serial.printf("Taken %d samples taken in %d s a frequency of %d Hz\n", nrSamples, WINDOW_TIME_SAMPLING_SECONDS, sampleFreq);
        Serial.printf("Avg in this window is %.2f\n",avg);
        xQueueSend(xAvgQueue, &avg, 0);
      }
    }
    //Using DMA to sample
    uint16_t dmaBuffer[1024];
    Serial.printf("Continue sampling with DMA a the frequency of %d",sampleFreq);
    setupDMA(sampleFreq);
    while (1){
        float avg = samplingAvgOnWindowDMA(sampleFreq, nrSamples,dmaBuffer, sizeof(dmaBuffer));
        Serial.printf("Taken %d samples taken in %d s a frequency of %d Hz\n", nrSamples, WINDOW_TIME_SAMPLING_SECONDS, sampleFreq);
        Serial.printf("Avg in this window is %.2f\n",avg);
        xQueueSend(xAvgQueue, &avg, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

}

//########################################
//            TaskWifi
//#########################################
void TaskWifi(void *pvParameters){
  Serial.println("\nConnecting to wifi ...");
  //WiFi.setSleep(WIFI_PS_MIN_MODEM); 
  if(SAVE_ENERGY)
    WiFi.setSleep(WIFI_PS_MAX_MODEM); //set energy saving max
    
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    //Serial.print(".");
  }
  Serial.println("Wifi is connected");

  while (1){
    //reconnection to wifi if signal lost
    while(WiFi.status() != WL_CONNECTED){
      Serial.println("Connection to wifi is lost, try to reconnect");
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
    if(SAVE_ENERGY)
      vTaskDelay(pdMS_TO_TICKS(5000));
    else
      vTaskDelay(pdMS_TO_TICKS(1000));
  }
}


//########################################
//           MQTT CLIENT TASK
//#########################################
void callback(char*, byte* , unsigned int );
const char* mqtt_server = "192.168.0.144";
const int mqtt_port = 1883;
const char* TEST_TOPIC_OUT = "topic/ping";
const char* TEST_TOPIC_IN = "topic/pong";


uint64_t timesend;

void TaskClientMQTT(void *pvParameters){
  //inizialization
  WiFiClient espClient;
  PubSubClient client(espClient);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  char msg[128];
  float avg;
  while (1){

    if(WiFi.status() == WL_CONNECTED){

        if(!client.connected()){
          Serial.println("Connecting to MQTT broker ...");

          if (client.connect("ESP32_Test")){
            Serial.println("Connected to MQTT broker");
            
            //topic pong
            client.subscribe(TEST_TOPIC_IN);

          }else{
            Serial.println(" Failed, retrying in 5 seconds...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue; 
        }   
      }
      client.loop();

      // check the queue for max 10 ms
      if (xQueueReceive(xAvgQueue, &avg, pdMS_TO_TICKS(10)) == pdPASS) {
        
        snprintf(msg, sizeof(msg),"{\"device\":\"ESP32_Test\",\"average\":%.2f}\n", avg);
        Serial.printf("Publishing message: %s",msg);
        timesend = micros();
        client.publish(TEST_TOPIC_OUT, msg);
        endWindowTime = micros();
        Serial.printf("The time of this window is %d ms\n", (endWindowTime-startWindowTime)/1000);
        startWindowTime = micros();
        }

    }
    
    if(SAVE_ENERGY)
      vTaskDelay(pdMS_TO_TICKS(WINDOW_TIME_SAMPLING_SECONDS*1000));
    else
      vTaskDelay(pdMS_TO_TICKS(1));
    
  }
}


// This function is executed when a message arrives on a subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  if(strcmp(topic,TEST_TOPIC_IN) == 0){
  uint64_t timerrt = (micros() - timesend)/1000;
  Serial.printf("\nTotal time RRT : %d ms\n", timerrt);
  Serial.printf("End to end latency of this system : %d ms\n", timerrt/2);

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Create a temporary buffer for the payload to safely null-terminate/parse
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
  }

}
