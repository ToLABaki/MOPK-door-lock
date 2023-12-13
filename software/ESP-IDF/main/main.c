#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "sd_spi.h"
#include "MFRC522.h"


static void mainTask(void *pvParameter);

#define HSPI_MISO   GPIO_NUM_12
#define HSPI_CLK    GPIO_NUM_14
#define HSPI_MOSI    GPIO_NUM_13

#define RELAY_PIN       GPIO_NUM_27


gpio_config_t io_conf_out = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = (1ULL<<RELAY_PIN),
    .pull_down_en = 1,
    .pull_up_en = 0

};

gpio_config_t io_conf_HSPI = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = (1ULL<<HSPI_MISO) | (1ULL<<HSPI_CLK) | (1ULL<<HSPI_MOSI),
    .pull_down_en = 0,
    .pull_up_en = 1

};


spi_bus_config_t buscfg={
    .miso_io_num=HSPI_MISO,
    .sclk_io_num=HSPI_CLK,
    .mosi_io_num=HSPI_MOSI,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000
    
};


esp_err_t spi_init(){
    esp_err_t ret = ESP_OK;
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if(ret != ESP_OK){
        printf("SPI INT ERR\n");
    }
    return ret;
}



MFRC522Ptr_t mfrc;

void app_main(void){
    gpio_config(&io_conf_HSPI);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    FILE *f = NULL;



    spi_init();
    if(sd_init() == ESP_OK){
        printf("SD INIT\n");
        f = fopen("/sdcard/FOO.TXT", "r");
        if(f == NULL){
            printf("File error!\n");
           
        }else{
            printf("File opened\n");
            fclose(f);
        }
    }
    gpio_config(&io_conf_out);
    gpio_set_level(RELAY_PIN, 0);

    mfrc = MFRC522_Init();
    PCD_Init(mfrc);

    xTaskCreatePinnedToCore(mainTask, "mainTask", 4096*2, NULL, tskIDLE_PRIORITY, NULL,0);


}




void mainTask(void *pvParameter){
    uint32_t i = 0, status = 0;
    while(1){
        
        if(PICC_IsNewCardPresent(mfrc) == 1){

            //printf("READING\n");
            vTaskDelay(50/portTICK_PERIOD_MS);
            printf("PICC_ReadCardSerial START\n");
            
            
            printf("PICC_ReadCardSerial END\n");
            printf("%d  %x-%x-%x-%x  %x\n",PICC_ReadCardSerial(mfrc) , mfrc->uid.uidByte[0], mfrc->uid.uidByte[1], mfrc->uid.uidByte[2], mfrc->uid.uidByte[3], mfrc->uid.sak );
            status = 1;
            i = 0;
        }else{
            printf("NO\n");
            i++;
            if(i == 2){
                status = 0;
                i = 0;
            }
        }

        if(status == 1){
            gpio_set_level(RELAY_PIN, 1);
        }else{
            gpio_set_level(RELAY_PIN, 0);
        }
        
        vTaskDelay(300/portTICK_PERIOD_MS);
        
    }
    vTaskDelete(NULL);
}