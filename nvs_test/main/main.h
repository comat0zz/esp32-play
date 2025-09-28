#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_
// -------------------------------------
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// -------------------------------------
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
// -------------------------------------
#endif /* MAIN_MAIN_H_ */

esp_err_t nvs_partition_init(void);
void nvs_get_info(void);