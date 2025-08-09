#include "w2812b.h"

static const char *TAG="W2812B";

// channel for tx_rmt
static rmt_channel_handle_t tx_handle;

// Set the times of transmission in a loop
static rmt_transmit_config_t transmit_config = {
    .loop_count = 0,
};

static rmt_encoder_handle_t    copy_encoder_handle=NULL;   // Pointer to set start 
static rmt_encoder_handle_t    data_encoder_handle=NULL;   // Pointer to data

// symnbol
static rmt_symbol_word_t reset_symbol;

//Data
uint8_t *data=NULL;
static volatile uint8_t isTransfered =0;


bool rmt_done(rmt_channel_handle_t tx_handle, const rmt_tx_done_event_data_t *edata, void *user_ctx)
{
    if(data && !isTransfered)
    {
        rmt_transmit(tx_handle, data_encoder_handle,(uint8_t *)data , LEDS*3, &transmit_config); 
        isTransfered=1;
        return false;
    }
    return true;
} 

rmt_tx_event_callbacks_t cbs = {
        .on_trans_done = rmt_done,
    };

void w2812b_setData(uint8_t *new_Data, uint8_t new_len)
{
    if (data) 
    { 
        free(data); 
        data = NULL; 
    } 
    // data = (uint8_t *)malloc(new_len*3);
    // if (!data) 
    // {
    //     ESP_LOGI(TAG,"Fail to allocate memory");
    //     data= NULL;
    //     return;
    // }
    data = (uint8_t *)malloc(LEDS*3);
    if (!data) 
    {
        ESP_LOGI(TAG,"Fail to allocate memory");
        data= NULL;
        return;
    }
    int copied_bytes = new_len*3;
    int remain_bytes = LEDS*3 - copied_bytes;
    memcpy(data, new_Data,copied_bytes);
    if(remain_bytes > 0)
    {
        memset(data + copied_bytes, 0x00, remain_bytes);
    }
    ESP_LOGI(TAG,"Set RMT data successfully");
}
void w2812b_Tx_Reset(void)
{
    isTransfered=0;
    esp_err_t ret = rmt_transmit(tx_handle, copy_encoder_handle, &reset_symbol, sizeof(reset_symbol), &transmit_config);
    if(ret !=ESP_OK) ESP_LOGE(TAG, "rmt_transmit failed: %s", esp_err_to_name(ret));  
}
void w2812b_init(int tx_gpio, uint32_t freq)
{
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = MODE_80MHZ,
        .gpio_num = tx_gpio,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .resolution_hz = freq,
        .intr_priority = 1,
    };
    float nano_to_tick = (1.0f /freq) * 1e9f;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config,&tx_handle));
    ESP_ERROR_CHECK(rmt_enable(tx_handle)); 
    rmt_copy_encoder_config_t encoder_config = {0};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&encoder_config,&copy_encoder_handle));

    reset_symbol.duration0 = 250/(int)nano_to_tick;
    reset_symbol.level0 = 0;
    reset_symbol.duration1 = 250/(int)nano_to_tick;
    reset_symbol.level1 = 0;

    rmt_bytes_encoder_config_t s = {
       .bit0.duration0 = 400/(int)nano_to_tick,
        .bit0.level0 = 1,
        .bit0.duration1 = 850/(int)nano_to_tick,
        .bit0.level1 = 0,
        .bit1.duration0 = 800/(int)nano_to_tick,
        .bit1.level0 = 1,
        .bit1.duration1 = 450/(int)nano_to_tick,
        .bit1.level1 = 0,
    };
    ESP_ERROR_CHECK( rmt_new_bytes_encoder(&s, &data_encoder_handle));
    ESP_ERROR_CHECK(rmt_tx_register_event_callbacks(tx_handle, &cbs, NULL));
    ESP_LOGI(TAG,"Initialize Tx RMT successfully");
}
