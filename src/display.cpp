#include <stdio.h>
#include <string.h>
#include "display.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "input.h"
#include "driver/i2c_master.h"

static i2c_master_dev_handle_t oled_dev_handle = NULL;

static const uint8_t font5x7[59][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // 65 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // 77 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
    {0x26, 0x49, 0x49, 0x49, 0x32}, // 83 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 87 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}  // 90 'Z'
};

static void oled_send_cmd(uint8_t cmd) {
    uint8_t buffer[2] = {0x00, cmd};
    i2c_master_transmit(oled_dev_handle, buffer, 2, -1);
}

static void oled_init(void) {
    i2c_master_bus_config_t bus_cfg;
    memset(&bus_cfg, 0, sizeof(bus_cfg));
    bus_cfg.i2c_port = I2C_PORT;
    bus_cfg.sda_io_num = I2C_SDA_PIN;
    bus_cfg.scl_io_num = I2C_SCL_PIN;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handle;
    i2c_new_master_bus(&bus_cfg, &bus_handle);

    i2c_device_config_t dev_cfg;
    memset(&dev_cfg, 0, sizeof(dev_cfg));
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = OLED_ADDR;
    dev_cfg.scl_speed_hz = 400000;

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
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        int idx = (c >= 32 && c <= 90) ? (c - 32) : 0;

        uint8_t buffer[6] = {0x40};
        memcpy(&buffer[1], font5x7[idx], 5);
        i2c_master_transmit(oled_dev_handle, buffer, 6, -1);

        uint8_t space[2] = {0x40, 0x00};
        i2c_master_transmit(oled_dev_handle, space, 2, -1);
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
    SensorData latest;
    memset(&latest, 0, sizeof(latest));
    latest.temperature = 25.4f;
    latest.humidity = 61.2f;
    latest.lightLevel = 24;

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