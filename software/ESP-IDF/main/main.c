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
#include "driver/i2c.h"

#include "sd_spi.h"
#include "MFRC522.h"


static void mainTask(void *pvParameter);

#define HSPI_MISO   GPIO_NUM_12
#define HSPI_CLK    GPIO_NUM_14
#define HSPI_MOSI    GPIO_NUM_13

#define RELAY_PIN       GPIO_NUM_27
#define BUZZER_PIN       GPIO_NUM_32

#define RTC_ADDR        0x68

static const char *TAG = "i2c-example";


gpio_config_t io_conf_out = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = (1ULL<<RELAY_PIN) | (1ULL<<BUZZER_PIN),
    .pull_down_en = 0,
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



uint8_t rx_data[8];

i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 21,
    .scl_io_num = 22,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 900000
};


	

	

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


    i2c_param_config(I2C_NUM_0, &conf);
    if(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0) != ESP_OK){
        printf("I2c error!\n");
    }

    

    xTaskCreatePinnedToCore(mainTask, "mainTask", 4096*2, NULL, tskIDLE_PRIORITY, NULL,0);


}




void mainTask(void *pvParameter){
    uint32_t i = 0, status = 0;
    i2c_cmd_handle_t cmd;
    uint8_t tmp;
    while(1){
        
        if(PICC_IsNewCardPresent(mfrc) == 1){

            //printf("READING\n");
            vTaskDelay(50/portTICK_PERIOD_MS);
            //printf("PICC_ReadCardSerial START\n");
            
            
            //printf("PICC_ReadCardSerial END\n");
            //printf("%d  %x-%x-%x-%x  %x\n",PICC_ReadCardSerial(mfrc) , mfrc->uid.uidByte[0], mfrc->uid.uidByte[1], mfrc->uid.uidByte[2], mfrc->uid.uidByte[3], mfrc->uid.sak );
            status = 1;
            i = 0;
        }else{
            //printf("NO\n");
            i++;
            if(i == 2){
                status = 0;
                i = 0;
            }
        }

        if(status == 1){
            gpio_set_level(RELAY_PIN, 1);
            //gpio_set_level(BUZZER_PIN, 1);
        }else{
            //gpio_set_level(BUZZER_PIN, 0);
            gpio_set_level(RELAY_PIN, 0);
        }

        tmp = 0x00;

        //cmd = i2c_cmd_link_create();
        //i2c_master_start(cmd);
        //i2c_master_write_byte(cmd, (RTC_ADDR << 1)| I2C_MASTER_WRITE ,1);
        //i2c_master_write(cmd, &tmp, 1, 1);
        //i2c_master_stop(cmd);
        ///i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);

        
        //i2c_master_start(cmd);

        //i2c_master_read(cmd, rx_data, 5, 1);
        //i2c_master_stop(cmd);
        //i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
        //i2c_cmd_link_delete(cmd);

        //i2c_master_write_to_device(I2C_NUM_0, RTC_ADDR, tmp, 1, 1000/portTICK_PERIOD_MS);   
        i2c_master_read_from_device(I2C_NUM_0, RTC_ADDR, rx_data, 8, 1000/portTICK_PERIOD_MS);
        
        ESP_LOG_BUFFER_HEX(TAG, rx_data, 8);
        

        

        
        
        vTaskDelay(300/portTICK_PERIOD_MS);
        
    }
    vTaskDelete(NULL);
}