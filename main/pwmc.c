#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <driver/ledc.h>
#include <esp_err.h>

#include "wifi_ledctrl.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT 
#define LEDC_FREQUENCY          (5000) 

//RED, GREEN, BLUE, LIGHT
int output_pins[4] = {0, 1, 2, 10}; 
int relay_pin = 3;
ledc_channel_t channels[4] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3};


void ledc_init() {

    int i;

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    for (i = 0; i < 4; i++) {

        ledc_channel_config_t ledc_channel = {
            .speed_mode     = LEDC_MODE,
            .channel        = channels[i],
            .timer_sel      = LEDC_TIMER,
//            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = output_pins[i],
            .duty           = 0, 
            .hpoint         = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }
    
    gpio_set_direction(relay_pin, GPIO_MODE_OUTPUT);
    
}

void set_duty(int chn, int duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, channels[chn], duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, channels[chn]));
}

void parse_command(char *command_string) {
    int i;
    char c_tokens[3][32];
    int c_args[3];
    char *tok = strtok(command_string, " ");

    for (i = 0; tok && i < 3; i++) {
        strcpy(c_tokens[i], tok);
        tok = strtok(NULL, " ");
    }

    if (!strncmp(c_tokens[0], "start", 5)) {
        ESP_LOGI(TAG, "starting led");
        gpio_set_level(relay_pin, 1);
    }
    else if (!strncmp(c_tokens[0], "stop", 4)) {
        ESP_LOGI(TAG, "stopping led");
        gpio_set_level(relay_pin, 0);
    }
    else if (!strncmp(c_tokens[0], "set", 3)) {
        tok = strtok(c_tokens[1], ",");
        for (i = 0; tok && i < 3; i++) {
            c_args[i] = atoi(tok);
            tok = strtok(NULL, ",");
        }
        
        for (i = 0; i < 3; i++) {
            set_duty(channels[i], c_args[i]);
        }
    }
    else if (!strncmp(c_tokens[0], "light", 5)) {
        set_duty(3, atoi(c_tokens[1]));
    }
    else {
        ESP_LOGI(TAG, "unknown command: %s", c_tokens[0]);
    }

}
