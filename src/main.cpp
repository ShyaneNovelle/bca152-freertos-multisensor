#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "motion.h"
#include "system_state.h"

#ifndef IS_TEST_BUILD
extern "C" void __attribute__((weak)) app_main(void) {
    rtos_objects_init();
    sensors_init();

    safe_log("\n--- Starting FreeRTOS Modular Multisensor Room Monitor ---");

    xTaskCreate(motion_task,  "MotionTask",  3072, NULL, 3, NULL);
    xTaskCreate(input_task,   "InputTask",   3072, NULL, 3, NULL);
    xTaskCreate(sensor_task,  "SensorTask",  3072, NULL, 2, NULL);
    xTaskCreate(alarm_task,   "AlarmTask",   3072, NULL, 2, NULL);
    xTaskCreate(display_task, "DisplayTask", 3072, NULL, 1, NULL);
}
#endif