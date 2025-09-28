#include "main.h"

const static char *TAG = "nvs_test";
const static char *NVS_NMSPC = "test_storage";
const static char *TEST_COUNTER = "counter";

void app_main(void)
{
    if(nvs_partition_init() == ESP_OK)
    {
        nvs_get_info();
        nvs_handle_t nvs_handle;
        uint8_t tc = 0;
        
        esp_err_t err = nvs_open(NVS_NMSPC, NVS_READWRITE, &nvs_handle);
       
        // Счетчик тестов, будет открывать запись строк, на 20 сотрет все нафиг
        err = nvs_get_u8(nvs_handle, TEST_COUNTER, &tc);
        if(err != ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_ERROR_CHECK(err);
        };
        
        tc++;
        ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, TEST_COUNTER, tc));
        
        switch(tc)
        {
            case 1:
                ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "Hello", "World!"));
                break;
            case 5:
                ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "Password", "qwerty12345"));
                break;
            case 10:
                ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "MD5Hash", "85064efb60a9601805dcea56ec5402f7"));
                break;
            case 15:
                ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "Goodbye", "Baby goodbye!"));
                break;
            default:
                break;
        };

        err = nvs_commit(nvs_handle);
        ESP_LOGI(TAG, "Write: Its %s!", ((err != ESP_OK)? "failed" : "done")); 

        // Возврат итератора для листалки пар
        nvs_iterator_t it = NULL;
        esp_err_t res = nvs_entry_find_in_handle(nvs_handle, NVS_TYPE_ANY, &it);
        if(res != ESP_OK)
        {
            ESP_LOGI(TAG, "NVS partition no read: %d (%s)", res, esp_err_to_name(res));

        }
        else
        {
            while(res == ESP_OK)
            {
                nvs_entry_info_t info;
                nvs_entry_info(it, &info); // Возвращает инфу для пары
                res = nvs_entry_next(&it); // Возвращает итератор следующей пары

                /*
                ESP_LOGI(TAG, "key '%s', type '%02x', namespace_name '%s'",
                    info.key,
                    info.type,
                    info.namespace_name
                ); */
                
                uint8_t out_int = 0;
                size_t required_size; 
                
                switch(info.type)
                {
                    case NVS_TYPE_STR:
                        nvs_get_str(nvs_handle, info.key, NULL, &required_size);
                        char* out_str = malloc(required_size);
                        nvs_get_str(nvs_handle, info.key, out_str, &required_size);
                        ESP_LOGI(TAG, "DATA: %s %s", info.key, out_str);
                        break;

                    default:
                        nvs_get_u8(nvs_handle, info.key, &out_int);
                        ESP_LOGI(TAG, "Test counter is %d", out_int);
                }; 
            };
        };
        nvs_release_iterator(it);
        nvs_close(nvs_handle);

        if(tc >= 20)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_LOGW(TAG, "Test was cleared! You are in the game! Expect a new round...");
            nvs_partition_init();
        };
    };    

    // Restart 
    for (int i = 5; i >= 0; i--)
    {
        ESP_LOGI(TAG, "Restarting in %d seconds...", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    };

    ESP_LOGI(TAG, "Restarting now.");
    esp_restart();
};

void nvs_get_info(void)
{
    nvs_stats_t nvs_stats;
    nvs_get_stats(NVS_DEFAULT_PART_NAME, &nvs_stats);
    ESP_LOGI(TAG, "Count: UsedEntries = (%lu), FreeEntries = (%lu), AvailableEntries = (%lu), AllEntries = (%lu)",
        nvs_stats.used_entries, 
        nvs_stats.free_entries, 
        nvs_stats.available_entries, 
        nvs_stats.total_entries
    );
};

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