#pragma once

#include "driver/gpio.h"

#define ENCODER_CLK GPIO_NUM_18
#define ENCODER_DT  GPIO_NUM_19
#define ENCODER_SW  GPIO_NUM_23

typedef enum {
    MODE_TEMPERATURE = 0,
    MODE_HUMIDITY,
    MODE_LIGHT,
    MODE_MOTION,
    MODE_COUNT
} DisplayMode;

DisplayMode nextDisplayMode(DisplayMode current);
DisplayMode previousDisplayMode(DisplayMode current);
void input_task(void *pvParameters);