#include "wifi.h"

#define WIFI_SSID "****"
#define WIFI_PASS "****"
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAXIMUM_RETRY 10

static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "wifi-module";
static int s_retry_num = 0;

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // Обработка любого события
    esp_event_handler_instance_t instance_any_id;
    // Обработка получения IP и успешности соединения
    esp_event_handler_instance_t instance_got_ip;

    // Регистрация обработчиков
    /*
        Любое событие при использовании Wi-Fi
    */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, 
        ESP_EVENT_ANY_ID, 
        &event_handler, 
        NULL, 
        &instance_any_id)
    );
    /*
        Это событие возникает, когда DHCP-клиент успешно получает IPV4-адрес
        от DHCP-сервера или когда IPV4-адрес изменяется. Событие означает, 
        что всё готово, и приложение может приступить к выполнению своих задач 
        (например, созданию сокетов).
        https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/wifi.html#ip-event-sta-got-ip
    */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, 
        IP_EVENT_STA_GOT_IP, 
        &event_handler, 
        NULL, 
        &instance_got_ip)
    );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            }
        }
    };
    /*
        Режим станции: в этом режиме  esp_wifi_start() 
        инициализирует внутренние данные станции, 
        при этом интерфейс станции готов к приёму 
        и передаче данных Wi-Fi. 
    */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    /*
        Полностью отключить режим сна модема. 
        Отключение увеличивает энергопотребление, 
        но минимизирует задержку получения данных Wi-Fi 
        в режиме реального времени.
    */
     ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
     ESP_ERROR_CHECK(esp_wifi_start());

     ESP_LOGI(TAG, "wifi_init_sta finished.");

     EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
          pdFALSE,
          pdFALSE,
          portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT)
    {
      ESP_LOGI(TAG, "connected to ap SSID: %s", WIFI_SSID);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGW(TAG, "Failed to connect to SSID: %s", WIFI_SSID);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
};

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    // Если станция запущена, подключаем ее к точке доступа
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    // Если дисконнект, то пробуем переподключится
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        if (s_retry_num < WIFI_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        };

        ESP_LOGI(TAG,"connect to the AP fail");
    }
    // Если подключились, то получим инфу об IP
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "You can connect now!\nIP Address: %d.%d.%d.%d\nSubnet mask: %d.%d.%d.%d\nGateway: %d.%d.%d.%d", 
            IP2STR(&event->ip_info.ip),
            IP2STR(&event->ip_info.netmask),
            IP2STR(&event->ip_info.gw)
        );
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
};