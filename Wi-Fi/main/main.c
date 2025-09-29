#include "main.h"

static const char *TAG = "wifi";

void app_main(void)
{
    wifi_ap_record_t info;
    ESP_ERROR_CHECK(nvs_partition_init());
    // Initialize the event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    // Initialize Wi-Fi default
    esp_netif_t *esp_netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_netif_set_hostname(esp_netif, "esp32 device"));

    wifi_init_sta();

    while (true) {
        int ret = esp_wifi_sta_get_ap_info(&info);
        ESP_LOGI(TAG, "esp_wifi_sta_get_ap_info: 0x%04x", ret);
        if(ret == 0)
        {
            ESP_LOGI(TAG, "SSID: %s", info.ssid);
        }
        else
        {
            wifi_init_sta();
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

esp_err_t nvs_partition_init(void)
{
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    };
    if(err == ESP_OK)
    {
        ESP_LOGI(TAG, "NVS partition initilized");
    } 
    else
    {
        ESP_LOGE(TAG, "NVS partition initialization error: %d (%s)", err, esp_err_to_name(err));
    };

    return err;
};