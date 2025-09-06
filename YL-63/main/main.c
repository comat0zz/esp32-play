#include <stdio.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"
#include "freertos/queue.h" 
#include "driver/gpio.h" 

#define WARN_LED_PIN GPIO_NUM_19
#define SCSS_LED_PIN GPIO_NUM_18
#define DATA_PIN GPIO_NUM_23
#define STACK_SIZE 4096

static const char* TAG = "YL-63";
static StaticQueue_t xStaticQueue;
QueueHandle_t xQueue;

typedef struct {
    uint8_t gpio;
    bool state;
} AlertLed_t;

#define QUEUE_LENGTH    10
#define QUEUE_ITEM_SIZE   sizeof( AlertLed_t )


static void vSignalToLed(void *pvParameters)
{
    AlertLed_t *artData = (AlertLed_t *)pvParameters;

    gpio_set_level(WARN_LED_PIN, 0); 
    gpio_set_level(SCSS_LED_PIN, 0);
    
    ESP_LOGI(TAG, "Led Active: %d State: %s", 
        artData->gpio,
        (artData->state ? "On": "Off")
    );
    gpio_set_level(artData->gpio, artData->state);
    vTaskDelete(NULL);
}

static void vSignalReceiver(void *pvParameters)
{
    AlertLed_t ledData;
    portBASE_TYPE xStatus;

    while(1) {

        if( uxQueueMessagesWaiting( xQueue ) != 0 )
        {
            ESP_LOGI(TAG, "Queue should have been empty!\r\n" );
        }

        xStatus = xQueueReceive(xQueue, &ledData, portMAX_DELAY);
        if( xStatus == pdPASS )
        {
            ESP_LOGI(TAG, "New message received: %d", ledData.gpio);
            xTaskCreate(vSignalToLed, "signalToLed", STACK_SIZE, &ledData, 5, NULL);
        }
        else
        {
            ESP_LOGI(TAG, "Could not receive from the queue.\r\n" );
        }
    }
}

static void vSignalSendQueue(void *pvParameters)
{
    uint8_t sensor_state;
    AlertLed_t ledData;
    portBASE_TYPE xStatus;

    if(xQueue != NULL)
    {   
        while(1)
        { 
            sensor_state = gpio_get_level(DATA_PIN);
            ESP_LOGI(TAG, "Sensor State: %d", sensor_state);

            ledData.gpio = sensor_state ? SCSS_LED_PIN : WARN_LED_PIN ;
            ledData.state = true;

            xStatus = xQueueSend(xQueue,  &ledData, pdMS_TO_TICKS(1000));
            if(xStatus != pdPASS) {
                ESP_LOGE(TAG, "Failed to adding message to queue!");
            }else{
                ESP_LOGI(TAG, "Send message to queue!");
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

void app_main(void)
{
    gpio_reset_pin(WARN_LED_PIN);
    gpio_reset_pin(SCSS_LED_PIN);
    gpio_set_direction(WARN_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(SCSS_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(DATA_PIN);
    gpio_set_direction(DATA_PIN, GPIO_MODE_INPUT);

    uint8_t ucQueueStorageArea[ QUEUE_LENGTH * QUEUE_ITEM_SIZE ];
    xQueue = xQueueCreateStatic(QUEUE_LENGTH, QUEUE_ITEM_SIZE, ucQueueStorageArea, &xStaticQueue);
    
    if(xQueue != NULL)
    {
        // Планировщик сам переключать будет
        xTaskCreate(vSignalSendQueue, "Send Sensor", STACK_SIZE, NULL, 1, NULL);
        xTaskCreate(vSignalReceiver, "Receiver", STACK_SIZE, NULL, 2, NULL );
    }
}