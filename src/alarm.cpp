#include "alarm.h"
#include "sensors.h"
#include "rtos_objects.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);

    SensorData data;
    while (1) {
        if (xQueueReceive(sensorQueue, &data, portMAX_DELAY) == pdTRUE) {
            AlarmState state = evaluateTemperature(data.temperature);
            if (state != ALARM_NORMAL) {
                gpio_set_level(BUZZER_PIN, 1);
            } else {
                gpio_set_level(BUZZER_PIN, 0);
            }
        }
    }
}