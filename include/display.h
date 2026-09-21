#pragma once

#include "driver/gpio.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_PIN GPIO_NUM_21
#define I2C_SCL_PIN GPIO_NUM_22
#define OLED_ADDR 0x3C

void display_task(void *pvParameters);