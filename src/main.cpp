#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "alarm.h"
#include "input.h"
#include "motion.h"
#include "display.h"

extern "C" void app_main(void) {
    rtos_objects_init();

    safe_log("Starting Modular Multisensor Room Monitor (Part XIII)...\n");

    sensors_init();

    xTaskCreate(motion_task,  "MotionTask",  2048, NULL, 3, NULL); 
    xTaskCreate(input_task,   "InputTask",   2048, NULL, 3, NULL); 
    xTaskCreate(sensor_task,  "SensorTask",  4096, NULL, 2, NULL); 
    xTaskCreate(alarm_task,   "AlarmTask",   2048, NULL, 2, NULL); 
    xTaskCreate(display_task, "DisplayTask", 4096, NULL, 1, NULL); 
}