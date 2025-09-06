#include <stdio.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
#include "driver/gpio.h" 

#define WARN_LED_PIN GPIO_NUM_19
#define SCSS_LED_PIN GPIO_NUM_18
#define DATA_PIN GPIO_NUM_23

#define STACK_SIZE 4096

static const char* TAG = "YL-63";

typedef struct {
    uint8_t gpio;
    bool state;
} AlertLed_t;

void vSignalToLed(void *pvParameters)
{
    gpio_set_level(WARN_LED_PIN, 0); 
    gpio_set_level(SCSS_LED_PIN, 0);
    AlertLed_t *artData = (AlertLed_t *)pvParameters;
    ESP_LOGI(TAG, "Led Active: %d State: %s", 
        artData->gpio,
        (artData->state ? "On": "Off")
    );
    gpio_set_level(artData->gpio, artData->state);
    vTaskDelete(NULL);
}

void app_main(void)
{
    uint8_t sensor_state;
    gpio_reset_pin(WARN_LED_PIN);
    gpio_reset_pin(SCSS_LED_PIN);
    gpio_set_direction(WARN_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(SCSS_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(DATA_PIN);
    gpio_set_direction(DATA_PIN, GPIO_MODE_INPUT);

    AlertLed_t ledData = {
        .gpio = SCSS_LED_PIN,
        .state = true 
    };
    xTaskCreate(vSignalToLed, "signalToLed", STACK_SIZE, &ledData, 5, NULL);

    while (1) {
        sensor_state = gpio_get_level(DATA_PIN);
        ESP_LOGI(TAG, "Sensor State: %d", sensor_state);

        if(sensor_state == 1) {
            ledData.gpio = SCSS_LED_PIN;
        }else{
            ledData.gpio = WARN_LED_PIN;
        }

        xTaskCreate(vSignalToLed, "signalToLed", STACK_SIZE, &ledData, 5, NULL);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}