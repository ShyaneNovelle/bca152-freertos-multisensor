#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#define DHT_PIN GPIO_NUM_15

// Function para maghulat sa signal level nga naay microsecond timeout
static int wait_or_timeout(int us_timeout, int expected_level) {
    int elapsed = 0;
    while (gpio_get_level(DHT_PIN) != expected_level) {
        if (elapsed++ > us_timeout) {
            return -1;
        }
        ets_delay_us(1);
    }
    return elapsed;
}

static esp_err_t read_dht22(float *temperature, float *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    // 1. ESP32 Start Signal: I-pull LOW og 20ms
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    // 2. I-pull HIGH og 40us
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(40);

    // 3. I-switch sa Input mode nga naay Pull-up Resistor
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT_PIN, GPIO_PULLUP_ONLY);

    // 4. DHT Response: Maghulat sa 80us LOW ug 80us HIGH
    if (wait_or_timeout(100, 0) < 0) return ESP_FAIL;
    if (wait_or_timeout(100, 1) < 0) return ESP_FAIL;
    if (wait_or_timeout(100, 0) < 0) return ESP_FAIL;

    // 5. Basahon ang 40 bits (5 bytes)
    for (int i = 0; i < 40; i++) {
        // Maghulat sa pagsugod sa bit (50us low)
        if (wait_or_timeout(100, 1) < 0) return ESP_FAIL;

        // Sukdon ang gidugayon sa HIGH signal
        int64_t start = esp_timer_get_time();
        if (wait_or_timeout(100, 0) < 0) return ESP_FAIL;
        int64_t duration = esp_timer_get_time() - start;

        // Kon mas taas sa 40us ang HIGH pulse, kini '1'; kon dili, '0'
        if (duration > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // 6. Checksum Verification
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (data[4] != (checksum & 0xFF)) {
        return ESP_FAIL;
    }

    // 7. Conversion ngadto sa Temperature ug Humidity
    int16_t raw_humidity = (data[0] << 8) | data[1];
    int16_t raw_temperature = ((data[2] & 0x7F) << 8) | data[3];
    if (data[2] & 0x80) {
        raw_temperature = -raw_temperature;
    }

    *humidity = raw_humidity / 10.0;
    *temperature = raw_temperature / 10.0;

    return ESP_OK;
}

void app_main(void)
{
    printf("DHT22 Sensor Reading Initialized...\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1) {
        float temp = 0.0;
        float hum = 0.0;

        if (read_dht22(&temp, &hum) == ESP_OK) {
            printf("Temperature: %.2f C\n", temp);
            printf("Humidity: %.2f %%\n", hum);
        } else {
            printf("Waiting for sensor data...\n");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}