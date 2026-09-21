#include <stdio.h>
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
    gpio_set_direction(ENCODER_CLK, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ENCODER_CLK, GPIO_PULLUP_ONLY);
    gpio_set_direction(ENCODER_DT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ENCODER_DT, GPIO_PULLUP_ONLY);

    int last_clk = gpio_get_level(ENCODER_CLK);
    DisplayMode current_mode = MODE_TEMPERATURE;
    char log_buf[64];

    while (1) {
        int current_clk = gpio_get_level(ENCODER_CLK);

        if (current_clk != last_clk && current_clk == 0) {
            if (gpio_get_level(ENCODER_DT) != current_clk) {
                current_mode = nextDisplayMode(current_mode);
                snprintf(log_buf, sizeof(log_buf), "[InputTask] Rotated CW -> Mode: %d\n", (int)current_mode);
            } else {
                current_mode = previousDisplayMode(current_mode);
                snprintf(log_buf, sizeof(log_buf), "[InputTask] Rotated CCW -> Mode: %d\n", (int)current_mode);
            }

            safe_log(log_buf);
            xQueueOverwrite(modeQueue, &current_mode);
        }

        last_clk = current_clk;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}