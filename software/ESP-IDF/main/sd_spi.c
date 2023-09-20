#include "sd_spi.h"

sdmmc_host_t host = {\
    .flags = SDMMC_HOST_FLAG_SPI | SDMMC_HOST_FLAG_DEINIT_ARG, \
    .slot = VSPI_HOST, \
    .max_freq_khz = 3000, \
    .io_voltage = 3.3f, \
    .init = &sdspi_host_init, \
    .set_bus_width = NULL, \
    .get_bus_width = NULL, \
    .set_bus_ddr_mode = NULL, \
    .set_card_clk = &sdspi_host_set_card_clk, \
    .do_transaction = &sdspi_host_do_transaction, \
    .deinit_p = &sdspi_host_remove_device, \
    .io_int_enable = &sdspi_host_io_int_enable, \
    .io_int_wait = &sdspi_host_io_int_wait, \
    .command_timeout_ms = 0, \
};

esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
};
sdmmc_card_t *card;


sdspi_device_config_t slot_config = {\
    .host_id   = VSPI_HOST, \
    .gpio_cs   = SD_CS, \
    .gpio_cd   = SDSPI_SLOT_NO_CD, \
    .gpio_wp   = SDSPI_SLOT_NO_WP, \
    .gpio_int  = GPIO_NUM_NC, \
};


    

esp_err_t sd_init(){
    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if(ret != ESP_OK){
        printf("SD SPI ERR\n");
        return ret;
    }
    return ret;

}


esp_err_t sd_deinit(){
    esp_err_t ret = esp_vfs_fat_sdcard_unmount("/sdcard", card);
    if(ret != ESP_OK){
        printf("SD SPI RM ERR\n");
        return ret;
    }
    return ret;

}