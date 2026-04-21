#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <math.h>

//OLED display
extern SSD1306Wire display;

uint64_t sendtime = 0;

// Key of the Think network
uint8_t appKey[] = { 0xA5, 0x52, 0xB8, 0xE2, 0x79, 0x5B, 0x2D, 0xF7, 0xC1, 0xA8, 0x39, 0x5C, 0xBA, 0xEB, 0x90, 0x7E };
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x07, 0x70, 0x25 }; 
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  




//ABP KEY (FOR THE LIBRARY)
uint8_t nwkSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint32_t devAddr =  ( uint32_t )0x00000000;


//Connection Parameters for LoRaWAN
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };
DeviceClass_t  loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 120000; // Invia ogni 120 secondi per il test
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

QueueHandle_t xAvgQueue;

char msg[32] = "Hello"; 


//Message To send
static void prepareTxFrame( uint8_t port )
{

    appDataSize = strlen(msg);
    memcpy(appData, msg, appDataSize);


}


void taskLoraTTN( void *pvParameters );
void taskCalculateAvg(void* pvParameters);


void setup() {
    Serial.begin(115200);
    xAvgQueue = xQueueCreate(5, sizeof(float));
    //init of board 
    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);

    //power on the display
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(100);

    display.init();
    display.setFont(ArialMT_Plain_10); // Scegliamo la grandezza del font
    display.clear();
    display.drawString(0, 0, "Heltec V3 Avviata!");
    display.display();

    xTaskCreatePinnedToCore(taskLoraTTN,"taskLoraTTN",10000,NULL,5,NULL,1);
    xTaskCreatePinnedToCore(taskCalculateAvg,"taskCalculateAvg",2048,NULL,1,NULL,1);
}

void loop() {
    vTaskDelete(NULL);
}

void taskCalculateAvg(void* pvParameters){
while (true){
    //get random data
    float avg = random(0, 409601) / 100.0;
    xQueueSend(xAvgQueue, &avg, 0);
    vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


void taskLoraTTN( void *pvParameters ) {
    while (1){
        switch( deviceState ) {
            case DEVICE_STATE_INIT: {
                        
                        LoRaWAN.init(loraWanClass, ACTIVE_REGION);//active region is in Platoformio file
                        deviceState = DEVICE_STATE_JOIN;
                        break;
                    }
            case DEVICE_STATE_JOIN: {
                display.clear();
                display.drawString(0, 0, "Connessione a TTN...");
                display.display();
                LoRaWAN.join();
                break;
            }
            case DEVICE_STATE_SEND: {
                Serial.println("Join to TTN successful, sending data...");
                float avg;
                if(xQueueReceive(xAvgQueue, &avg, pdMS_TO_TICKS(100)) == pdPASS){
                    snprintf(msg, sizeof(msg), "%.2f", avg);
                }
                Serial.printf(msg);
                Serial.println();
                prepareTxFrame( appPort );
                display.clear();
                display.drawString(0, 0, "Connesso a TTN!");
                display.drawString(0, 16, "Invio dati");
                display.display();
                LoRaWAN.send();
                deviceState = DEVICE_STATE_CYCLE;
                break;
            }
            case DEVICE_STATE_CYCLE: {
                display.clear();
                display.drawString(0, 0, "Prossimo invio tra ");
                display.drawString(0, 16, String(appTxDutyCycle/1000) + " secondi");
                display.display();
                txDutyCycleTime = appTxDutyCycle;
                LoRaWAN.cycle(txDutyCycleTime);
                deviceState = DEVICE_STATE_SLEEP;
                break;
            }
            case DEVICE_STATE_SLEEP: {
                LoRaWAN.sleep(loraWanClass);
                break;
            }
            default: {
                deviceState = DEVICE_STATE_INIT;
                break;
            }
        }
    }
    
}

