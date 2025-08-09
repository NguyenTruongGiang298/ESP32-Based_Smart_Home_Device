#ifndef PWM_H
#define PWM_H
#include "driver/ledc.h"

#define LOWSPEED_CH0_GPIO           4
#define LOWSPEED_CH1_GPIO           5
#define HIGHSPEED_CH2_GPIO          18
#define HIGHSPEED_CH3_GPIO          19

void timer_config(ledc_timer_bit_t bit_num,unsigned int frequency, ledc_mode_t speed_mode, ledc_timer_t timer_num);
void pwm_config(ledc_channel_config_t *ch,ledc_channel_t channel, int duty, ledc_mode_t speed, ledc_timer_t timer_num, int gpio);
void update_duty(ledc_channel_config_t *ch,uint32_t newDuty);

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

#endif
