#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

// Hardware Pin Configuration
#define DHT_PIN GPIO_NUM_15
#define LDR_CHANNEL ADC_CHANNEL_6 
#define PIR_PIN GPIO_NUM_27
#define BUZZER_PIN GPIO_NUM_14

#define ENCODER_CLK GPIO_NUM_18
#define ENCODER_DT  GPIO_NUM_19
#define ENCODER_SW  GPIO_NUM_23

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_PIN GPIO_NUM_21
#define I2C_SCL_PIN GPIO_NUM_22
#define OLED_ADDR 0x3C

// Section 35: Event Group Bit Definitions
#define EVENT_ACTIVE (1 << 0) // BIT0
#define EVENT_MOTION (1 << 1) // BIT1
#define EVENT_ALARM  (1 << 2) // BIT2

static EventGroupHandle_t systemEvents = NULL;

// Alarm States & Thresholds
typedef enum {
    ALARM_NORMAL = 0,
    ALARM_LOW_TEMPERATURE,
    ALARM_HIGH_TEMPERATURE
} AlarmState;

#define TEMP_LOW_THRESHOLD   18.0f
#define TEMP_HIGH_THRESHOLD  28.0f

// Display Modes
typedef enum {
    MODE_TEMPERATURE = 0,
    MODE_HUMIDITY,
    MODE_LIGHT,
    MODE_MOTION,
    MODE_COUNT
} DisplayMode;

// Sensor Data Structure
typedef struct {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
    AlarmState alarm;
} SensorData;

static QueueHandle_t sensorQueue = NULL;
static QueueHandle_t modeQueue = NULL;
static adc_oneshot_unit_handle_t adc1_handle;
static i2c_master_dev_handle_t oled_dev_handle = NULL;

// Pure Testable Function
AlarmState evaluateTemperature(float temperature) {
    if (temperature < TEMP_LOW_THRESHOLD) {
        return ALARM_LOW_TEMPERATURE;
    } else if (temperature > TEMP_HIGH_THRESHOLD) {
        return ALARM_HIGH_TEMPERATURE;
    }
    return ALARM_NORMAL;
}

// 5x7 Font Table
static const uint8_t font5x7[][5] = {
    [' ' - 32] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['%' - 32] = {0x23, 0x13, 0x08, 0x64, 0x62},
    ['.' - 32] = {0x00, 0x60, 0x60, 0x00, 0x00},
    ['0' - 32] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1' - 32] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2' - 32] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3' - 32] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4' - 32] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5' - 32] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6' - 32] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7' - 32] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8' - 32] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9' - 32] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    ['A' - 32] = {0x7C, 0x12, 0x11, 0x12, 0x7C},
    ['C' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D' - 32] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['G' - 32] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['H' - 32] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['I' - 32] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['L' - 32] = {0x7F, 0x40, 0x40, 0x40, 0x40},
    ['M' - 32] = {0x7F, 0x02, 0x04, 0x02, 0x7F},
    ['N' - 32] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['R' - 32] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S' - 32] = {0x26, 0x49, 0x49, 0x49, 0x32},
    ['T' - 32] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U' - 32] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    ['V' - 32] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    ['W' - 32] = {0x7F, 0x20, 0x18, 0x20, 0x7F},
    ['Y' - 32] = {0x07, 0x08, 0x70, 0x08, 0x07},
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

void motion_task(void *pvParameters) {
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIR_PIN, GPIO_PULLDOWN_ONLY);

    int64_t last_motion_time = esp_timer_get_time();
    const int64_t timeout_us = 15LL * 1000000LL;

    while (1) {
        int motion = gpio_get_level(PIR_PIN);

        if (motion == 1) {
            last_motion_time = esp_timer_get_time();
            xEventGroupSetBits(systemEvents, EVENT_MOTION | EVENT_ACTIVE);
        } else {
            xEventGroupClearBits(systemEvents, EVENT_MOTION);
            if ((esp_timer_get_time() - last_motion_time) > timeout_us) {
                xEventGroupClearBits(systemEvents, EVENT_ACTIVE);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void sensor_task(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(2000);

    while (1) {
        SensorData data = {0};
        data.lightLevel = read_ldr_percentage();
        data.motionDetected = (xEventGroupGetBits(systemEvents) & EVENT_MOTION) != 0;

        if (read_dht22(&data.temperature, &data.humidity) == ESP_OK) {
            data.alarm = evaluateTemperature(data.temperature);

            if (data.alarm != ALARM_NORMAL) {
                xEventGroupSetBits(systemEvents, EVENT_ALARM);
            } else {
                xEventGroupClearBits(systemEvents, EVENT_ALARM);
            }

            xQueueSend(sensorQueue, &data, pdMS_TO_TICKS(100));
        }

        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

void alarm_task(void *pvParameters) {
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(
            systemEvents,
            EVENT_ACTIVE | EVENT_ALARM,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(100)
        );

        if ((bits & EVENT_ACTIVE) && (bits & EVENT_ALARM)) {
            gpio_set_level(BUZZER_PIN, 1);
        } else {
            gpio_set_level(BUZZER_PIN, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void input_task(void *pvParameters) {
    gpio_set_direction(ENCODER_CLK, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ENCODER_CLK, GPIO_PULLUP_ONLY);
    gpio_set_direction(ENCODER_DT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ENCODER_DT, GPIO_PULLUP_ONLY);

    int last_clk = gpio_get_level(ENCODER_CLK);
    DisplayMode current_mode = MODE_TEMPERATURE;

    while (1) {
        int current_clk = gpio_get_level(ENCODER_CLK);

        if (current_clk != last_clk && current_clk == 0) {
            if (gpio_get_level(ENCODER_DT) != current_clk) {
                current_mode = (current_mode + 1) % MODE_COUNT;
            } else {
                if (current_mode == 0) current_mode = MODE_COUNT - 1;
                else current_mode--;
            }
            xQueueOverwrite(modeQueue, &current_mode);
        }

        last_clk = current_clk;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void render_screen(DisplayMode mode, const SensorData *data) {
    char val_str[16];
    oled_clear();
    oled_draw_string(20, 1, "ROOM MONITOR");

    switch (mode) {
        case MODE_TEMPERATURE:
            oled_draw_string(25, 3, "TEMPERATURE");
            snprintf(val_str, sizeof(val_str), "%.1f C", data->temperature);
            oled_draw_string(45, 5, val_str);
            break;
        case MODE_HUMIDITY:
            oled_draw_string(35, 3, "HUMIDITY");
            snprintf(val_str, sizeof(val_str), "%.1f %%", data->humidity);
            oled_draw_string(42, 5, val_str);
            break;
        case MODE_LIGHT:
            oled_draw_string(28, 3, "LIGHT LEVEL");
            snprintf(val_str, sizeof(val_str), "%d %%", data->lightLevel);
            oled_draw_string(50, 5, val_str);
            break;
        case MODE_MOTION:
            oled_draw_string(40, 3, "MOTION");
            oled_draw_string(30, 5, data->motionDetected ? "DETECTED" : "NO MOTION");
            break;
        default:
            break;
    }
}

void display_task(void *pvParameters) {
    SensorData latest = { .temperature = 25.4, .humidity = 61.2, .lightLevel = 24 };
    DisplayMode active_mode = MODE_TEMPERATURE;
    bool was_active = true;

    oled_init();
    render_screen(active_mode, &latest);

    while (1) {
        EventBits_t bits = xEventGroupGetBits(systemEvents);
        bool is_active = (bits & EVENT_ACTIVE) != 0;
        bool need_refresh = false;

        if (is_active != was_active) {
            was_active = is_active;
            if (!is_active) {
                oled_clear();
            } else {
                need_refresh = true;
            }
        }

        DisplayMode new_mode;
        if (xQueueReceive(modeQueue, &new_mode, pdMS_TO_TICKS(50)) == pdPASS) {
            if (new_mode != active_mode) {
                active_mode = new_mode;
                if (is_active) need_refresh = true;
            }
        }

        SensorData new_data;
        if (xQueueReceive(sensorQueue, &new_data, 0) == pdPASS) {
            latest = new_data;
            if (is_active) need_refresh = true;
        }

        if (is_active && need_refresh) {
            render_screen(active_mode, &latest);
        }
    }
}

void app_main(void) {
    printf("Starting Multisensor with Event Group Signaling...\n");

    adc_oneshot_unit_init_cfg_t init_config1 = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, LDR_CHANNEL, &config);

    // Section 35: FreeRTOS Event Group Creation
    systemEvents = xEventGroupCreate();
    xEventGroupSetBits(systemEvents, EVENT_ACTIVE);

    sensorQueue = xQueueCreate(5, sizeof(SensorData));
    modeQueue = xQueueCreate(1, sizeof(DisplayMode));

    DisplayMode initial_mode = MODE_TEMPERATURE;
    xQueueSend(modeQueue, &initial_mode, 0);

    if (systemEvents != NULL && sensorQueue != NULL && modeQueue != NULL) {
        xTaskCreate(alarm_task, "AlarmTask", 2048, NULL, 3, NULL);
        xTaskCreate(motion_task, "MotionTask", 2048, NULL, 3, NULL);
        xTaskCreate(input_task, "InputTask", 2048, NULL, 3, NULL);
        xTaskCreate(sensor_task, "SensorTask", 4096, NULL, 2, NULL);
        xTaskCreate(display_task, "DisplayTask", 4096, NULL, 1, NULL);
    }
}