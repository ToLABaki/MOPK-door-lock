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

#define VSPI_MISO   GPIO_NUM_19
#define VSPI_CLK    GPIO_NUM_18
#define VSPI_MOSI    GPIO_NUM_23


spi_bus_config_t buscfg={
.miso_io_num=VSPI_MISO,
.sclk_io_num=VSPI_CLK,
.mosi_io_num=VSPI_MOSI,
.quadwp_io_num = -1,
.quadhd_io_num = -1,
};


esp_err_t spi_init(){
    esp_err_t ret = ESP_OK;
    ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    
    if(ret != ESP_OK){
           printf("SPI INT ERR\n");
    }
    return ret;
}


void app_main(void){
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
    INIT_MFRC522();
    PCD_Init1();

    xTaskCreatePinnedToCore(mainTask, "mainTask", 4096*2, NULL, tskIDLE_PRIORITY, NULL,0);


}




void mainTask(void *pvParameter){
    while(1){
        if(1){
            printf("%d\n",PICC_IsNewCardPresent());
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    }
    vTaskDelete(NULL);
}