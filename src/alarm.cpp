#include "alarm.h"
#include "rtos_objects.h"
#include "sensors.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#ifndef BUZZER_PIN
#define BUZZER_PIN GPIO_NUM_27
#endif

#define BUZZER_PWM_TIMER       LEDC_TIMER_0
#define BUZZER_PWM_MODE        LEDC_LOW_SPEED_MODE
#define BUZZER_PWM_CHANNEL     LEDC_CHANNEL_0
#define BUZZER_PWM_DUTY_RES    LEDC_TIMER_10_BIT 
#define BUZZER_PWM_FREQ_HZ     2000              

static void buzzer_init(void) {
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode       = BUZZER_PWM_MODE;
    timer_conf.duty_resolution  = BUZZER_PWM_DUTY_RES;
    timer_conf.timer_num        = BUZZER_PWM_TIMER;
    timer_conf.freq_hz          = BUZZER_PWM_FREQ_HZ;
    timer_conf.clk_cfg          = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {};
    ch_conf.gpio_num   = BUZZER_PIN;
    ch_conf.speed_mode = BUZZER_PWM_MODE;
    ch_conf.channel    = BUZZER_PWM_CHANNEL;
    ch_conf.intr_type  = LEDC_INTR_DISABLE;
    ch_conf.timer_sel  = BUZZER_PWM_TIMER;
    ch_conf.duty       = 0;
    ch_conf.hpoint     = 0;
    ledc_channel_config(&ch_conf);
}

static void buzzer_start_tone(void) {
    ledc_set_duty(BUZZER_PWM_MODE, BUZZER_PWM_CHANNEL, 512);
    ledc_update_duty(BUZZER_PWM_MODE, BUZZER_PWM_CHANNEL);
}

static void buzzer_stop_tone(void) {
    ledc_set_duty(BUZZER_PWM_MODE, BUZZER_PWM_CHANNEL, 0);
    ledc_update_duty(BUZZER_PWM_MODE, BUZZER_PWM_CHANNEL);
}

AlarmState evaluateTemperature(float temperature) {
    if (temperature < TEMP_LOW_THRESHOLD) {
        return ALARM_LOW_TEMPERATURE;
    } else if (temperature > TEMP_HIGH_THRESHOLD) {
        return ALARM_HIGH_TEMPERATURE;
    }
    return ALARM_NORMAL;
}

void alarm_task(void *pvParameters) {
    buzzer_init();

    SensorData data;
    memset(&data, 0, sizeof(SensorData));

    for (;;) {
        if (sensorQueue != NULL) {
            xQueuePeek(sensorQueue, &data, portMAX_DELAY);
        }

        bool is_active = true;
        if (systemEvents != NULL) {
            EventBits_t bits = xEventGroupGetBits(systemEvents);
            is_active = (bits & EVENT_ACTIVE) != 0;
        }

        AlarmState state = evaluateTemperature(data.temperature);

        if (is_active && (state != ALARM_NORMAL)) {
            buzzer_start_tone();
            vTaskDelay(pdMS_TO_TICKS(150));
            buzzer_stop_tone();
            vTaskDelay(pdMS_TO_TICKS(150));
        } else {
            buzzer_stop_tone();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}