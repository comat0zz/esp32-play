#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_
//-------------------------------------------------------------
#include <stdio.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//-------------------------------------------------------------
#include "wifi.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_netif.h"
//-------------------------------------------------------------
#endif /* MAIN_MAIN_H_ */

esp_err_t nvs_partition_init(void);
esp_err_t get_current_ip(void);