#include <stdio.h>
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
//#include "esp_log.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
#include "driver/gpio.h" 

#define RED_LED_PIN GPIO_NUM_21
#define BLUE_LED_PIN GPIO_NUM_17
#define GREEN_LED_PIN GPIO_NUM_16

// static const char* TAG = "led-ping";

void app_main(void)
{
  gpio_reset_pin(RED_LED_PIN);
  gpio_reset_pin(BLUE_LED_PIN);
  gpio_reset_pin(GREEN_LED_PIN);
  gpio_set_direction(RED_LED_PIN, GPIO_MODE_OUTPUT);
  gpio_set_direction(BLUE_LED_PIN, GPIO_MODE_OUTPUT);
  gpio_set_direction(GREEN_LED_PIN, GPIO_MODE_OUTPUT);

  while (1) { 
       gpio_set_level(RED_LED_PIN, 1);  // Включаем светодиод 
       vTaskDelay(500 / portTICK_PERIOD_MS);  // Задержка 1 секунда 

       gpio_set_level(RED_LED_PIN, 0);  // Выключаем светодиод 
       vTaskDelay(500 / portTICK_PERIOD_MS);  // Задержка 1 секунда 


       gpio_set_level(BLUE_LED_PIN, 1);  // Включаем светодиод 
       vTaskDelay(500 / portTICK_PERIOD_MS);  // Задержка 1 секунда 

       gpio_set_level(BLUE_LED_PIN, 0);  // Выключаем светодиод 
       vTaskDelay(500 / portTICK_PERIOD_MS);  // Задержка 1 секунда 


       gpio_set_level(GREEN_LED_PIN, 1);  // Включаем светодиод 
       vTaskDelay(500 / portTICK_PERIOD_MS);  // Задержка 1 секунда 

       gpio_set_level(GREEN_LED_PIN, 0);  // Выключаем светодиод 
       vTaskDelay(500 / portTICK_PERIOD_MS);  // Задержка 1 секунда 
   } 

}
