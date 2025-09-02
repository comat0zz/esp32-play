#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
#include "driver/gpio.h" 
#include "driver/ledc.h"
#include <unistd.h> 
#include <esp_random.h>

#include "pitches.h"

// Разводка спикера: красный на ножку, черный на землю
#define SPKR_DATA_PIN GPIO_NUM_23 
#define SPKR_MELODY_LENGTH 200 // Количество нот в серии
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))

#define CONFIG_BEEP_TIMER    LEDC_TIMER_0          // timer index (0-3)
#define CONFIG_BEEP_CHANNEL  LEDC_CHANNEL_0        // channel index (0-7)
#define CONFIG_BEEP_DRES     LEDC_TIMER_8_BIT      // resolution of PWM duty
#define CONFIG_BEEP_MODE     LEDC_HIGH_SPEED_MODE  // timer mode

static const char* logTAG = "Speaker";

// Генерация случайного числа в диапазоне от Min до Max (включительно)
int getRandomNumberInRange(int Min, int Max) {
  return Min + (esp_random() % (Max - Min + 1));
}

// Генерация случайного числа от 0 до N-1
int getRandomNumber(int N) {
  return esp_random() % N;
}

void app_main(void)
{
  uint16_t melody[] = {
    NOTE_B0,NOTE_C1,NOTE_CS1,NOTE_D1,NOTE_DS1,NOTE_E1,
    NOTE_F1,NOTE_FS1,NOTE_G1,NOTE_GS1,NOTE_A1,NOTE_AS1,
    NOTE_B1,NOTE_C2,NOTE_CS2,NOTE_D2,NOTE_DS2,NOTE_E2,
    NOTE_F2,NOTE_FS2,NOTE_G2,NOTE_GS2,NOTE_A2,NOTE_AS2,
    NOTE_B2,NOTE_C3,NOTE_CS3,NOTE_D3,NOTE_DS3,NOTE_E3,
    NOTE_F3,NOTE_FS3,NOTE_G3,NOTE_GS3,NOTE_A3,NOTE_AS3,
    NOTE_B3,NOTE_C4,NOTE_CS4,NOTE_D4,NOTE_DS4,NOTE_E4,
    NOTE_F4,NOTE_FS4,NOTE_G4,NOTE_GS4,NOTE_A4,NOTE_AS4,
    NOTE_B4,NOTE_C5,NOTE_CS5,NOTE_D5,NOTE_DS5,NOTE_E5,
    NOTE_F5,NOTE_FS5,NOTE_G5,NOTE_GS5,NOTE_A5,NOTE_AS5,
    NOTE_B5,NOTE_C6,NOTE_CS6,NOTE_D6,NOTE_DS6,NOTE_E6,
    NOTE_F6,NOTE_FS6,NOTE_G6,NOTE_GS6,NOTE_A6,NOTE_AS6,
    NOTE_B6,NOTE_C7,NOTE_CS7,NOTE_D7,NOTE_DS7,NOTE_E7,
    NOTE_F7,NOTE_FS7,NOTE_G7,NOTE_GS7,NOTE_A7,NOTE_AS7};

  size_t numberOfNotes = ARRAY_SIZE(melody);

  uint16_t noteDuration;

  ledc_timer_config_t ledc_timer;
  ledc_timer.duty_resolution = LEDC_TIMER_8_BIT;  // Разрешение таймера 8 бит: это значения от 0 до 255
  ledc_timer.freq_hz = 256;                      // Частота ШИМ - 1 кГц
  ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;   // Выбираем высокоскоростную группу таймеров и каналов
  ledc_timer.timer_num = LEDC_TIMER_0;            // Пусть будет первый таймер из 4 возможных
  ledc_timer.clk_cfg = LEDC_AUTO_CLK;             // Выбор опорной частоты таймера оставим на совести производителя
  ledc_timer.deconfigure = 0;                     // Расконфигурацию делать не нужно
  // Настраиваем таймер
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer)); 

  ledc_channel_config_t ledc_channel;
  ledc_channel.channel = LEDC_CHANNEL_0;          // Канал 0
  ledc_channel.gpio_num = SPKR_DATA_PIN;          // Для вывода сигнала выберем GPIO
  ledc_channel.intr_type = LEDC_INTR_DISABLE,     // Прерывание не требуется
  ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE; // Выбираем высокоскоростную группу таймеров и каналов
  ledc_channel.timer_sel = LEDC_TIMER_0;          // Используем ранее настроенный таймер 0 (той же группы)
  ledc_channel.duty = 13;                          // Начальное заполнение цикла - 0%
  ledc_channel.hpoint = 0;                        // Относительная фаза
  ledc_channel.flags.output_invert = 1;           // Инвертирование сигнала на выходе
  ledc_channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD; // этой хуйни нет в примерах!
  // Настраиваем канал
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  while(1){
    ESP_LOGI(logTAG, "New melody for you <3 :)");
    for (uint16_t iNote = 0; iNote < SPKR_MELODY_LENGTH; iNote++) 
    {
        // Чтобы рассчитать длительность ноты
        // Если будет деление на ноль -> guru panic
        noteDuration = 1500 / getRandomNumberInRange(2, 5);
        // Тащим текущий код ноты рандомно
        int note_idx = getRandomNumber(numberOfNotes);

        ESP_ERROR_CHECK(ledc_set_freq(CONFIG_BEEP_MODE, CONFIG_BEEP_TIMER, melody[note_idx]));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, noteDuration));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
        
        // Минимальное время между нотами, длительность ноты
        ESP_ERROR_CHECK(ledc_update_duty(CONFIG_BEEP_MODE, LEDC_CHANNEL_0));
        vTaskDelay(pdMS_TO_TICKS(noteDuration * 1.30));
    }

    // Серия закончена, отключаем звук
    ESP_ERROR_CHECK(ledc_set_duty(CONFIG_BEEP_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(ledc_update_duty(CONFIG_BEEP_MODE, LEDC_CHANNEL_0));
    vTaskDelay(pdMS_TO_TICKS(3000));
  }  
}
