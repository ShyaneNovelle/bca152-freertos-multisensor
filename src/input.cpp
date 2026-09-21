#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "input.h"
#include "rtos_objects.h"

DisplayMode nextDisplayMode(DisplayMode current) {
    return (DisplayMode)((current + 1) % MODE_COUNT);
}

DisplayMode previousDisplayMode(DisplayMode current) {
    if (current == 0) {
        return (DisplayMode)(MODE_COUNT - 1);
    }
    return (DisplayMode)(current - 1);
}

void input_task(void *pvParameters) {
    gpio_reset_pin(ENCODER_CLK);
    gpio_reset_pin(ENCODER_DT);
    gpio_set_direction(ENCODER_CLK, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ENCODER_CLK, GPIO_PULLUP_ONLY);
    gpio_set_direction(ENCODER_DT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ENCODER_DT, GPIO_PULLUP_ONLY);

    int last_clk = gpio_get_level(ENCODER_CLK);
    DisplayMode current_mode = MODE_TEMPERATURE;
    char log_buf[64];

    for (;;) {
        int current_clk = gpio_get_level(ENCODER_CLK);

        if (current_clk != last_clk && current_clk == 0) {
            if (gpio_get_level(ENCODER_DT) != current_clk) {
                current_mode = nextDisplayMode(current_mode);
                snprintf(log_buf, sizeof(log_buf), "[InputTask] Rotated CW -> Mode: %d", (int)current_mode);
            } else {
                current_mode = previousDisplayMode(current_mode);
                snprintf(log_buf, sizeof(log_buf), "[InputTask] Rotated CCW -> Mode: %d", (int)current_mode);
            }

            safe_log(log_buf);

            if (modeQueue != NULL) {
                xQueueOverwrite(modeQueue, &current_mode);
            }
        }

        last_clk = current_clk;
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}