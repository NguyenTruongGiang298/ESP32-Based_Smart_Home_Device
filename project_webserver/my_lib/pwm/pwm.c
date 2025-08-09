#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "esp_err.h"

void timer_config(ledc_timer_bit_t bit_num,unsigned int frequency, ledc_mode_t speed_mode, ledc_timer_t timer_num)
{   
     ledc_timer_config_t timer = 
     {
        .duty_resolution = bit_num,
        .clk_cfg = LEDC_AUTO_CLK,
        .freq_hz = frequency,
        .speed_mode = speed_mode,
        .timer_num = timer_num,
    };
    ledc_timer_config(&timer);
}   
void pwm_config(ledc_channel_config_t *ch,ledc_channel_t channel, int duty, ledc_mode_t speed, ledc_timer_t timer_num, int gpio)
{
    ch->channel    = channel;
    ch->duty       = duty;    /* 0 ->2 ** resolution -1*/
    ch->gpio_num   = gpio;
    ch->speed_mode = speed;
    ch->hpoint     = 0;
    ch->timer_sel  = timer_num;
    ch->intr_type  = LEDC_INTR_DISABLE;
    ch->flags.output_invert = 0;
    ledc_channel_config(ch);
}

void update_duty(ledc_channel_config_t *ch,uint32_t newDuty)
{
    ledc_set_duty(ch->speed_mode, ch->channel, newDuty);
    ledc_update_duty(ch->speed_mode, ch->channel);
}

/*
#define ESP_INTR_FLAG_LEVEL1        (1<<1)  ///< Accept a Level 1 interrupt vector (lowest priority)
#define ESP_INTR_FLAG_LEVEL2        (1<<2)  ///< Accept a Level 2 interrupt vector
#define ESP_INTR_FLAG_LEVEL3        (1<<3)  ///< Accept a Level 3 interrupt vector
#define ESP_INTR_FLAG_LEVEL4        (1<<4)  ///< Accept a Level 4 interrupt vector
#define ESP_INTR_FLAG_LEVEL5        (1<<5)  ///< Accept a Level 5 interrupt vector
#define ESP_INTR_FLAG_LEVEL6        (1<<6)  ///< Accept a Level 6 interrupt vector
#define ESP_INTR_FLAG_NMI           (1<<7)  ///< Accept a Level 7 interrupt vector (highest priority)
#define ESP_INTR_FLAG_SHARED        (1<<8)  ///< Interrupt can be shared between ISRs
#define ESP_INTR_FLAG_EDGE          (1<<9)  ///< Edge-triggered interrupt
#define ESP_INTR_FLAG_IRAM          (1<<10) ///< ISR can be called if cache is disabled
#define ESP_INTR_FLAG_INTRDISABLED  (1<<11) ///< Return with this interrupt disabled
*/