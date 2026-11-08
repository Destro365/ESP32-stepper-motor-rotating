#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "STEPPER";

#define IN1 GPIO_NUM_14
#define IN2 GPIO_NUM_27
#define IN3 GPIO_NUM_26
#define IN4 GPIO_NUM_25

#define STEPS_PER_REV 2048

static const gpio_num_t pins[4] = {IN1, IN2, IN3, IN4};
static const uint8_t full_step[4] = {0b0011, 0b0110, 0b1100, 0b1001};
static int step_index = 0;

static void stepper_write(uint8_t pattern)
{
    for (int i = 0; i < 4; i++) {
        gpio_set_level(pins[i], (pattern >> i) & 1);
    }
}

static void stepper_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << IN1) | (1ULL << IN2) | (1ULL << IN3) | (1ULL << IN4),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    stepper_write(0);
}

static void stepper_move(int32_t steps, uint32_t step_ms)
{
    int dir = (steps > 0) ? 1 : -1;
    TickType_t period = pdMS_TO_TICKS(step_ms);
    if (period == 0) period = 1;
    TickType_t last_wake = xTaskGetTickCount();

    for (int32_t s = 0; s < abs(steps); s++) {
        step_index = (step_index + dir + 4) % 4;
        stepper_write(full_step[step_index]);
        xTaskDelayUntil(&last_wake, period);
    }
    stepper_write(0);
}

static void stepper_task(void *arg)
{
    stepper_init();

    while (1) {
        ESP_LOGI(TAG, "Naprijed");
        stepper_move(STEPS_PER_REV, 3);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Natrag");
        stepper_move(-STEPS_PER_REV, 3);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    xTaskCreate(stepper_task, "stepper_task", 4096, NULL, 5, NULL);
}
