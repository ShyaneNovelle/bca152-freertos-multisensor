#pragma once

#include <stdbool.h>
#include "driver/gpio.h"
#include "alarm.h"

#define DHT_PIN GPIO_NUM_15
#define LDR_CHANNEL ADC_CHANNEL_6 // GPIO 34

typedef struct {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
    AlarmState alarm;
} SensorData;

void sensors_init(void);
void sensor_task(void *pvParameters);