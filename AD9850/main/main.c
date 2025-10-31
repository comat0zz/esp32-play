/* 
Datasheet: https://static.chipdip.ru/lib/143/DOC000143901.pdf
3V3 or 5v
Опорный генератор 125MHz, частота генерации от 0,029 Гц до 62,5 МГц
Выходной импеданс составляет минимум 50 кОм, типично 120 кОм.
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define PIN_NUM_DATA 23
#define PIN_NUM_W_CLK 18
#define PIN_NUM_FQ_UD 5 
#define PIN_NUM_RESET 19

#define AD9850_REF_FREQ 125000000 // Опорная частота AD9850 (125MHz)
#define BITS2V32 4294967295U // 2^32 - 1

static const char *TAG = "AD9850";

void GPIO_pulse_pin(gpio_num_t pin)
{
    gpio_set_level(pin, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(pin, 0);
}

void GPIO_init(void)
{
    gpio_reset_pin(PIN_NUM_DATA);
    gpio_reset_pin(PIN_NUM_W_CLK);
    gpio_reset_pin(PIN_NUM_FQ_UD);
    gpio_reset_pin(PIN_NUM_RESET);

    gpio_set_direction(PIN_NUM_DATA, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_W_CLK, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_FQ_UD, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RESET, GPIO_MODE_OUTPUT);

    GPIO_pulse_pin(PIN_NUM_RESET);
    GPIO_pulse_pin(PIN_NUM_W_CLK);
    GPIO_pulse_pin(PIN_NUM_FQ_UD); // this pulse enables serial mode - Datasheet page 12 figure 10 
    
    ESP_LOGI(TAG, "GPIO initialized successfully");
}

void AD9850_send_byte(uint8_t byte)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        bool bit_val = (byte >> i) & 1; 
        gpio_set_level(PIN_NUM_DATA, bit_val); 
        GPIO_pulse_pin(PIN_NUM_W_CLK);
    }
}

void AD9850_set_freq_and_phase(float freq_word, float phase)
{
    ESP_LOGI(TAG, "Setting frequency %u Hz with phase %u degrees", (uint32_t)freq_word, (uint32_t)phase);

    gpio_set_level(PIN_NUM_FQ_UD, 0); 
    uint32_t frequency = (uint32_t)((freq_word / AD9850_REF_FREQ) * BITS2V32);
    AD9850_send_byte(frequency & 0xFF);
    AD9850_send_byte((frequency >> 8) & 0xFF);
    AD9850_send_byte((frequency >> 16) & 0xFF);
    AD9850_send_byte((frequency >> 24) & 0xFF);
    if(phase > 0)
    {
        uint8_t phase_word = (uint8_t)(((phase / 360.0f) * 4294967296.0f));
        // Фаза нужна для синхронизации двух и более устройств
        // хз как тут тестить с одним
        // |= (phase << 3)
    }
    AD9850_send_byte(0);
    GPIO_pulse_pin(PIN_NUM_FQ_UD);
    ESP_LOGI(TAG, "Data sent successfully");
}

void AD9850_Reset(void)
{
    gpio_set_level(PIN_NUM_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(PIN_NUM_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    AD9850_set_freq_and_phase(0, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
}

void AD9850_Task(void *pvParameters)
{
    AD9850_Reset();
    float step_hz = 100000.0f; // 100 KHz
    uint32_t max_hz = 40 * 1000 * 1000; // 40 MHz, у меня выдает 44 примерно в максимуме
    float curr_hz = 0;

    while(1)
    {
        if(curr_hz >= (float)max_hz)
        {
            curr_hz = (float)step_hz;
        }

        ESP_LOGI(TAG, "Updating frequency and phase...");
        AD9850_set_freq_and_phase(curr_hz, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        curr_hz += (float)step_hz;
    }
}

void app_main(void)
{
    GPIO_init(); 
    xTaskCreate(AD9850_Task, "AD9850_Task", configMINIMAL_STACK_SIZE*2, NULL, tskIDLE_PRIORITY+1, NULL);
}