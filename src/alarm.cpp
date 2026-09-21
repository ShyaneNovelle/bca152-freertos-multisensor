#include "alarm.h"
#include "rtos_objects.h"
#include "driver/gpio.h"

AlarmState evaluateTemperature(float temperature) {
    if (temperature < TEMP_LOW_THRESHOLD) {
        return ALARM_LOW_TEMPERATURE;
    } else if (temperature > TEMP_HIGH_THRESHOLD) {
        return ALARM_HIGH_TEMPERATURE;
    }
    return ALARM_NORMAL;
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