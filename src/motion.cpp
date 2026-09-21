#include "motion.h"
#include "rtos_objects.h"
#include "esp_timer.h"

void motion_task(void *pvParameters) {
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIR_PIN, GPIO_PULLDOWN_ONLY);

    int64_t last_motion_time = esp_timer_get_time();
    const int64_t timeout_us = 15LL * 1000000LL;
    bool was_motion = false;

    while (1) {
        int motion = gpio_get_level(PIR_PIN);

        if (motion == 1) {
            last_motion_time = esp_timer_get_time();
            xEventGroupSetBits(systemEvents, EVENT_MOTION | EVENT_ACTIVE);
            if (!was_motion) {
                safe_log("[MotionTask] Motion Detected! State -> ACTIVE\n");
                was_motion = true;
            }
        } else {
            was_motion = false;
            xEventGroupClearBits(systemEvents, EVENT_MOTION);
            if ((esp_timer_get_time() - last_motion_time) > timeout_us) {
                if (xEventGroupGetBits(systemEvents) & EVENT_ACTIVE) {
                    xEventGroupClearBits(systemEvents, EVENT_ACTIVE);
                    safe_log("[MotionTask] Inactivity Timeout (15s)! State -> INACTIVE\n");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}