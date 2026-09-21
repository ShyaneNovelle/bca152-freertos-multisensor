#include <unity.h>
#include "alarm.h"
#include "input.h"
#include "system_state.h"

#include "../src/system_state.cpp"

DisplayMode nextDisplayMode(DisplayMode current) {
    return (DisplayMode)((current + 1) % MODE_COUNT);
}

DisplayMode previousDisplayMode(DisplayMode current) {
    if (current == 0) {
        return (DisplayMode)(MODE_COUNT - 1);
    }
    return (DisplayMode)(current - 1);
}

AlarmState evaluateTemperature(float temperature) {
    if (temperature < 18.0f) {
        return ALARM_LOW_TEMPERATURE;
    } else if (temperature > 30.0f) {
        return ALARM_HIGH_TEMPERATURE;
    }
    return ALARM_NORMAL;
}

void setUp(void) {}
void tearDown(void) {}

void test_temp_below_lower_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_LOW_TEMPERATURE, evaluateTemperature(15.0f));
}

void test_temp_exactly_lower_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(18.0f));
}

void test_temp_normal_value(void) {
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(24.5f));
}

void test_temp_exactly_upper_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(30.0f));
}

void test_temp_above_upper_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_HIGH_TEMPERATURE, evaluateTemperature(35.5f));
}

void test_display_forward_transition(void) {
    TEST_ASSERT_EQUAL(MODE_HUMIDITY, nextDisplayMode(MODE_TEMPERATURE));
}

void test_display_reverse_transition(void) {
    TEST_ASSERT_EQUAL(MODE_TEMPERATURE, previousDisplayMode(MODE_HUMIDITY));
}

void test_display_forward_wraparound(void) {
    TEST_ASSERT_EQUAL(MODE_TEMPERATURE, nextDisplayMode(MODE_MOTION));
}

void test_display_reverse_wraparound(void) {
    TEST_ASSERT_EQUAL(MODE_MOTION, previousDisplayMode(MODE_TEMPERATURE));
}

void test_state_active_no_timeout(void) {
    TEST_ASSERT_EQUAL(STATE_ACTIVE, evaluateSystemState(STATE_ACTIVE, false, false));
}

void test_state_active_timeout(void) {
    TEST_ASSERT_EQUAL(STATE_INACTIVE, evaluateSystemState(STATE_ACTIVE, false, true));
}

void test_state_inactive_no_motion(void) {
    TEST_ASSERT_EQUAL(STATE_INACTIVE, evaluateSystemState(STATE_INACTIVE, false, false));
}

void test_state_inactive_motion(void) {
    TEST_ASSERT_EQUAL(STATE_ACTIVE, evaluateSystemState(STATE_INACTIVE, true, false));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_temp_below_lower_threshold);
    RUN_TEST(test_temp_exactly_lower_threshold);
    RUN_TEST(test_temp_normal_value);
    RUN_TEST(test_temp_exactly_upper_threshold);
    RUN_TEST(test_temp_above_upper_threshold);

    RUN_TEST(test_display_forward_transition);
    RUN_TEST(test_display_reverse_transition);
    RUN_TEST(test_display_forward_wraparound);
    RUN_TEST(test_display_reverse_wraparound);

    RUN_TEST(test_state_active_no_timeout);
    RUN_TEST(test_state_active_timeout);
    RUN_TEST(test_state_inactive_no_motion);
    RUN_TEST(test_state_inactive_motion);

    return UNITY_END();
}