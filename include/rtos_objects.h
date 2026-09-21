#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#define EVENT_ACTIVE (1 << 0) 
#define EVENT_MOTION (1 << 1) 
#define EVENT_ALARM  (1 << 2) 

extern EventGroupHandle_t systemEvents;
extern SemaphoreHandle_t serialMutex;
extern QueueHandle_t sensorQueue;
extern QueueHandle_t modeQueue;

void rtos_objects_init(void);
void safe_log(const char *msg);