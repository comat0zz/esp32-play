#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define JOY_CHX ADC_CHANNEL_4 // GPIO32
#define JOY_CHY ADC_CHANNEL_5 // GPIO33
#define JOY_SW_PIN GPIO_NUM_18

const static char *TAG = "joy";

static int adc_raw[2];
static int voltage[2];
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

void app_main(void) {
 
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    adc_cali_handle_t adc1_cali_chan1_handle = NULL;

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    // Инициализирует блок АЦП
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        /*
            Битовая ширина — это разрешение канала. Оно варьируется от 9 до 13 бит. 
            Чем выше разрешение, тем точнее измерение, но преобразование соответствующего
            канала занимает больше времени.
        */
        .bitwidth = ADC_BITWIDTH_12,
        /*
            atten — это ослабление напряжения, используемое перед подачей на вход АЦП. 
            Используется ослабление 12 дБ, что позволяет измерять напряжения до 2450 мВ. 
        */
        .atten = ADC_ATTEN_DB_12, // 150mV - 2450mV
    };
    // Инициализация канала
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_CHX, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_CHY, &config));

    // Каллибровка каналов
    adc_calibration_init(ADC_UNIT_1, JOY_CHX, ADC_ATTEN_DB_12, &adc1_cali_chan0_handle);
    adc_calibration_init(ADC_UNIT_1, JOY_CHY, ADC_ATTEN_DB_12, &adc1_cali_chan1_handle);

    gpio_set_direction(JOY_SW_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(JOY_SW_PIN);
    
    while (1) {
        int button_state = gpio_get_level(JOY_SW_PIN);
        // Используется для чтения необработанных данных из определённого канала
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_CHX, &adc_raw[0]));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_CHY, &adc_raw[1]));

        // Преобразует данные с канала в напряжение, используя калибровочные данные канала
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0], &voltage[0]));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan1_handle, adc_raw[1], &voltage[1]));

        ESP_LOGI(TAG, "ADC%d: [%d : %d] Channels Raw Data(X.Y): %d.%d Voltage(X.Y): %d.%d Button: %s", 
            ADC_UNIT_1 + 1, 
            JOY_CHX, JOY_CHY,
            adc_raw[0], adc_raw[1], 
            voltage[0], voltage[1],
            button_state ? "Pressed" : "Not Pressed"
        );

        vTaskDelay(pdMS_TO_TICKS(1000));
    } 
}

/*---------------------------------------------------------------
    Калибровка ADC
    Функция калибровки adc_cali_create_scheme_curve_fitting 
    или adc_cali_create_scheme_line_fitting в зависимости от того, 
    какая схема поддерживается соответствующей платой ESP32.

    В ESP32 аналого-цифровой преобразователь (ADC) сравнивает входное аналоговое напряжение
    с опорным и определяет каждый бит выходного цифрового результата. 
    Конструктивно опорное напряжение ADC для ESP32 составляет 1100 мВ. 
    Однако истинное опорное напряжение может варьироваться от 1000 до 1200 мВ 
    в зависимости от микросхемы. 
    
    Калибровка ADC используется для минимизации влияния разницы опорных напряжений
    и получения более точных выходных результатов.

    ADC ESP32 поддерживает два типа схем калибровки:
        - Схема подгонки калибровочной линии ADC
        - Схема подгонки калибровочной кривой ADC
    https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/adc_calibration.html#calibration-scheme-creation
---------------------------------------------------------------*/
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}