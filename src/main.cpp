#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include <unity.h>

#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "motion.h"
#include "system_state.h"

static void test_temp_below_lower(void) { TEST_ASSERT_EQUAL(ALARM_LOW_TEMPERATURE, evaluateTemperature(15.0f)); }
static void test_temp_exact_lower(void) { TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(18.0f)); }
static void test_temp_normal(void)      { TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(24.5f)); }
static void test_temp_exact_upper(void) { TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(30.0f)); }
static void test_temp_above_upper(void) { TEST_ASSERT_EQUAL(ALARM_HIGH_TEMPERATURE, evaluateTemperature(35.5f)); }

static void test_disp_forward(void)     { TEST_ASSERT_EQUAL(MODE_HUMIDITY, nextDisplayMode(MODE_TEMPERATURE)); }
static void test_disp_reverse(void)     { TEST_ASSERT_EQUAL(MODE_TEMPERATURE, previousDisplayMode(MODE_HUMIDITY)); }
static void test_disp_wrap_fwd(void)    { TEST_ASSERT_EQUAL(MODE_TEMPERATURE, nextDisplayMode(MODE_MOTION)); }
static void test_disp_wrap_rev(void)    { TEST_ASSERT_EQUAL(MODE_MOTION, previousDisplayMode(MODE_TEMPERATURE)); }

static void test_st_active_no_timeout(void)  { TEST_ASSERT_EQUAL(STATE_ACTIVE, evaluateSystemState(STATE_ACTIVE, false, false)); }
static void test_st_active_timeout(void)     { TEST_ASSERT_EQUAL(STATE_INACTIVE, evaluateSystemState(STATE_ACTIVE, false, true)); }
static void test_st_inactive_no_motion(void) { TEST_ASSERT_EQUAL(STATE_INACTIVE, evaluateSystemState(STATE_INACTIVE, false, false)); }
static void test_st_inactive_motion(void)    { TEST_ASSERT_EQUAL(STATE_ACTIVE, evaluateSystemState(STATE_INACTIVE, true, false)); }

void run_embedded_unit_tests(void) {
    printf("\n==================================================\n");
    printf("        RUNNING PART XIV: 13 UNIT TESTS           \n");
    printf("==================================================\n");
    UNITY_BEGIN();

    RUN_TEST(test_temp_below_lower);
    RUN_TEST(test_temp_exact_lower);
    RUN_TEST(test_temp_normal);
    RUN_TEST(test_temp_exact_upper);
    RUN_TEST(test_temp_above_upper);

    RUN_TEST(test_disp_forward);
    RUN_TEST(test_disp_reverse);
    RUN_TEST(test_disp_wrap_fwd);
    RUN_TEST(test_disp_wrap_rev);

    RUN_TEST(test_st_active_no_timeout);
    RUN_TEST(test_st_active_timeout);
    RUN_TEST(test_st_inactive_no_motion);
    RUN_TEST(test_st_inactive_motion);

    UNITY_END();
    printf("==================================================\n\n");
}

extern "C" void app_main(void) {
    rtos_objects_init();

    run_embedded_unit_tests();

    safe_log("Starting Modular Multisensor Room Monitor (Part XIII/XIV)...\n");

    sensors_init();

    xTaskCreatePinnedToCore(motion_task, "MotionTask", 2048, NULL, 3, NULL, 1);

    xTaskCreatePinnedToCore(input_task, "InputTask", 2048, NULL, 3, NULL, 1);

    xTaskCreatePinnedToCore(sensor_task, "SensorTask", 3072, NULL, 2, NULL, 0);

    xTaskCreatePinnedToCore(alarm_task, "AlarmTask", 2048, NULL, 2, NULL, 0);

    xTaskCreatePinnedToCore(display_task, "DisplayTask", 4096, NULL, 1, NULL, 1);
}