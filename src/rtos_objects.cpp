#include <stdio.h>
#include "rtos_objects.h"
#include "input.h"
#include "sensors.h"

EventGroupHandle_t systemEvents = NULL;
SemaphoreHandle_t serialMutex = NULL;
QueueHandle_t sensorQueue = NULL;
QueueHandle_t modeQueue = NULL;

void rtos_objects_init(void) {
    serialMutex = xSemaphoreCreateMutex();
    systemEvents = xEventGroupCreate();
    xEventGroupSetBits(systemEvents, EVENT_ACTIVE);

    sensorQueue = xQueueCreate(5, sizeof(SensorData));
    modeQueue = xQueueCreate(1, sizeof(DisplayMode));

    DisplayMode initial_mode = MODE_TEMPERATURE;
    xQueueSend(modeQueue, &initial_mode, 0);
}

void safe_log(const char *msg) {
    if (serialMutex != NULL) {
        if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
            printf("%s", msg);
            xSemaphoreGive(serialMutex);
        }
    }
}