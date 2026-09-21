#pragma once

#include "driver/gpio.h"

#define PIR_PIN GPIO_NUM_27

void motion_task(void *pvParameters);