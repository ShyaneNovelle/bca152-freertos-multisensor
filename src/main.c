#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#define DHT_PIN GPIO_NUM_15
#define LDR_CHANNEL ADC_CHANNEL_6 

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_PIN GPIO_NUM_21
#define I2C_SCL_PIN GPIO_NUM_22
#define OLED_ADDR 0x3C

typedef struct {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
} SensorData;

static QueueHandle_t sensorQueue = NULL;
static adc_oneshot_unit_handle_t adc1_handle;
static i2c_master_dev_handle_t oled_dev_handle = NULL;

static const uint8_t font5x7[][5] = {
    [' ' - 32] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['.' - 32] = {0x00, 0x60, 0x60, 0x00, 0x00},
    ['0' - 32] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1' - 32] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2' - 32] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3' - 32] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4' - 32] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5' - 32] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['A' - 32] = {0x7C, 0x12, 0x11, 0x12, 0x7C},
    ['C' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['E' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['I' - 32] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['M' - 32] = {0x7F, 0x02, 0x04, 0x02, 0x7F},
    ['N' - 32] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['R' - 32] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['T' - 32] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U' - 32] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
};

static void oled_send_cmd(uint8_t cmd) {
    uint8_t buffer[2] = {0x00, cmd};
    i2c_master_transmit(oled_dev_handle, buffer, 2, -1);
}

static void oled_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    i2c_new_master_bus(&bus_cfg, &bus_handle);

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_ADDR,
        .scl_speed_hz = 400000,
    };
    i2c_master_bus_add_device(bus_handle, &dev_cfg, &oled_dev_handle);

    oled_send_cmd(0xAE); 
    oled_send_cmd(0x20); oled_send_cmd(0x00); 
    oled_send_cmd(0x40); 
    oled_send_cmd(0xA1); 
    oled_send_cmd(0xC8); 
    oled_send_cmd(0x88); 
    oled_send_cmd(0xAF); 
}

static void oled_clear(void) {
    uint8_t data[129];
    data[0] = 0x40;
    memset(&data[1], 0x00, 128);

    for (int page = 0; page < 8; page++) {
        oled_send_cmd(0xB0 + page);
        oled_send_cmd(0x00);
        oled_send_cmd(0x10);
        i2c_master_transmit(oled_dev_handle, data, 129, -1);
    }
}

static void oled_draw_string(int x, int page, const char *str) {
    oled_send_cmd(0xB0 + page);
    oled_send_cmd(x & 0x0F);
    oled_send_cmd(0x10 | ((x >> 4) & 0x0F));

    while (*str) {
        char c = *str++;
        uint8_t buffer[6] = {0x40};
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        int idx = (c >= 32 && c <= 90) ? (c - 32) : 0;
        memcpy(&buffer[1], font5x7[idx], 5);
        i2c_master_transmit(oled_dev_handle, buffer, 6, -1);

        uint8_t space[2] = {0x40, 0x00};
        i2c_master_transmit(oled_dev_handle, space, 2, -1);
    }
}

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
        return (raw * 100) / 4095;
    }
    return 0;
}

void sensor_task(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(2000);

    while (1) {
        SensorData data = {0};
        data.lightLevel = read_ldr_percentage();
        data.motionDetected = false;

        if (read_dht22(&data.temperature, &data.humidity) == ESP_OK) {
            xQueueSend(sensorQueue, &data, pdMS_TO_TICKS(100));
        }

        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

void display_task(void *pvParameters) {
    SensorData receivedData;
    char temp_str[16];

    oled_init();
    oled_clear();

    while (1) {
        if (xQueueReceive(sensorQueue, &receivedData, portMAX_DELAY) == pdPASS) {
            oled_clear();
            oled_draw_string(20, 1, "ROOM MONITOR");
            oled_draw_string(25, 3, "TEMPERATURE");
            snprintf(temp_str, sizeof(temp_str), "%.1f C", receivedData.temperature);
            oled_draw_string(45, 5, temp_str);
        }
    }
}

void app_main(void) {
    printf("Starting Multisensor OLED System...\n");

    adc_oneshot_unit_init_cfg_t init_config1 = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, LDR_CHANNEL, &config);

    sensorQueue = xQueueCreate(5, sizeof(SensorData));

    if (sensorQueue != NULL) {
        xTaskCreate(sensor_task, "SensorTask", 4096, NULL, 2, NULL);
        xTaskCreate(display_task, "DisplayTask", 4096, NULL, 1, NULL);
    }
}