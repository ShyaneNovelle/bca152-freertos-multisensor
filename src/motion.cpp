#include "motion.h"
#include "rtos_objects.h"
#include "system_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define MOTION_INACTIVITY_TIMEOUT_MS 15000

void motion_task(void *pvParameters) {
    gpio_reset_pin(PIR_PIN);
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);

    TickType_t last_motion_tick = xTaskGetTickCount();
    SystemState state = STATE_ACTIVE;

    for (;;) {
        int pir_val = gpio_get_level(PIR_PIN);
        bool motion_detected = (pir_val == 1);

        if (motion_detected) {
            last_motion_tick = xTaskGetTickCount();
            if (systemEvents != NULL) {
                xEventGroupSetBits(systemEvents, EVENT_MOTION);
            }
        } else {
            if (systemEvents != NULL) {
                xEventGroupClearBits(systemEvents, EVENT_MOTION);
            }
        }

        bool timeout_occurred =
            (xTaskGetTickCount() - last_motion_tick) >= pdMS_TO_TICKS(MOTION_INACTIVITY_TIMEOUT_MS);

        SystemState new_state = evaluateSystemState(state, motion_detected, timeout_occurred);

        if (new_state != state) {
            if (new_state == STATE_ACTIVE) {
                if (systemEvents != NULL) {
                    xEventGroupSetBits(systemEvents, EVENT_ACTIVE);
                }
                safe_log("[MotionTask] Motion Detected! System -> ACTIVE");
            } else {
                if (systemEvents != NULL) {
                    xEventGroupClearBits(systemEvents, EVENT_ACTIVE);
                }
                safe_log("[MotionTask] Inactivity timeout (15s)! System -> INACTIVE");
            }
            state = new_state;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}