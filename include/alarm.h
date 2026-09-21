#pragma once

#define BUZZER_PIN GPIO_NUM_14
#define TEMP_LOW_THRESHOLD   18.0f
#define TEMP_HIGH_THRESHOLD  30.0f

typedef enum {
    ALARM_NORMAL = 0,
    ALARM_LOW_TEMPERATURE,
    ALARM_HIGH_TEMPERATURE
} AlarmState;

AlarmState evaluateTemperature(float temperature);
void alarm_task(void *pvParameters);