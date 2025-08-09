#ifndef W2812B_H
#define W2812B_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ==== Clock Source Macro ====
#define MODE_80MHZ  SOC_MOD_CLK_APB        // 80MHz từ APB bus
#define MODE_1MHZ   SOC_MOD_CLK_REF_TICK   // ~1MHz từ Ref Tick
#define LEDS 12


void w2812b_setData(uint8_t *new_Data, uint8_t new_len);
void w2812b_init(int tx_gpio, uint32_t freq);
void tx_Callback(void *cb);
void w2812b_Tx_Reset(void);


#endif