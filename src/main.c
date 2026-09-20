#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("\n\n==============================\n");
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");
    printf("==============================\n\n");
    fflush(stdout);

    while (1) {
        printf("BCA152 FreeRTOS Multisensor running...\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}