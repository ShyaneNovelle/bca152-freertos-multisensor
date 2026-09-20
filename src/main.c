#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#define DHT_PIN GPIO_NUM_15
#define LDR_CHANNEL ADC_CHANNEL_6 

static adc_oneshot_unit_handle_t adc1_handle;

static int wait_or_timeout(int us_timeout, int expected_level) {
    int elapsed = 0;
    while (gpio_get_level(DHT_PIN) != expected_level) {
        if (elapsed++ > us_timeout) return -1;
        ets_delay_us(1);
    }
    return elapsed;
}

static esp_err_t read_dht22(float *temperature, float *humidity) {
    uint8_t data[5] = {0};

    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(40);

    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT_PIN, GPIO_PULLUP_ONLY);

    if (wait_or_timeout(100, 0) < 0) return ESP_FAIL;
    if (wait_or_timeout(100, 1) < 0) return ESP_FAIL;
    if (wait_or_timeout(100, 0) < 0) return ESP_FAIL;

    for (int i = 0; i < 40; i++) {
        if (wait_or_timeout(100, 1) < 0) return ESP_FAIL;
        int64_t start = esp_timer_get_time();
        if (wait_or_timeout(100, 0) < 0) return ESP_FAIL;
        if ((esp_timer_get_time() - start) > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) return ESP_FAIL;

    int16_t raw_humidity = (data[0] << 8) | data[1];
    int16_t raw_temperature = ((data[2] & 0x7F) << 8) | data[3];
    if (data[2] & 0x80) raw_temperature = -raw_temperature;

    *humidity = raw_humidity / 10.0;
    *temperature = raw_temperature / 10.0;
    return ESP_OK;
}

static int read_ldr_percentage(void) {
    int raw = 0;
    if (adc_oneshot_read(adc1_handle, LDR_CHANNEL, &raw) == ESP_OK) {
        int percentage = (raw * 100) / 4095;
        return percentage;
    }
    return 0;
}

void sensor_task(void *pvParameters)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(2000);

    while (1) {
        float temp = 0.0;
        float hum = 0.0;
        int light = read_ldr_percentage();

        if (read_dht22(&temp, &hum) == ESP_OK) {
            printf("[SensorTask] Temp: %.2f C | Humidity: %.2f %% | Light: %d %%\n", temp, hum, light);
        } else {
            printf("[SensorTask] Light: %d %% (Waiting for DHT22...)\n", light);
        }

        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

void app_main(void)
{
    printf("Starting Multisensor System...\n");

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, LDR_CHANNEL, &config);

    xTaskCreate(sensor_task, "SensorTask", 4096, NULL, 2, NULL);
}