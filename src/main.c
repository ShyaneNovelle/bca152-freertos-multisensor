#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Task A definition
void taskA(void *pvParameters)
{
    while (1) {
        printf("Task A running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task B definition
void taskB(void *pvParameters)
{
    while (1) {
        printf("Task B running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    // Paghimo sa duha ka tasks
    xTaskCreate(taskA, "Task_A", 2048, NULL, 1, NULL);
    xTaskCreate(taskB, "Task_B", 2048, NULL, 1, NULL);
}