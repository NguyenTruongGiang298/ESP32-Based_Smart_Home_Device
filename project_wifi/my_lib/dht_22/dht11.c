#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE // sử dụng đầy đủ log
#include "dht11.h"


static const char *TAG="DHT22";

#define INIT_OK 2
#define DATA_OK 1
#define DHT22_TIMEOUT_ERROR -1
#define DHT22_CHECKSUM_ERROR -2
#define DHT22_INIT_ERROR -3

void ERROR(int response)
{
    switch (response)
    {
    case DHT22_TIMEOUT_ERROR:
        ESP_LOGI(TAG, "Sensor Timeout");
        break;
    case DHT22_CHECKSUM_ERROR:
        ESP_LOGI(TAG, "CheckSum error");
        break;
    case DATA_OK:
        ESP_LOGI(TAG, "Sucessuflly receive Data");
        break;
    default:
        ESP_LOGI(TAG, "Unknown error");
    }
}


int getSignalLevel(int timeout ,int state, int gpio_num)
{
    int count = 0;
    while (gpio_get_level(gpio_num) == state)
    {
        if (count > timeout) return DHT22_TIMEOUT_ERROR;
        ++count;
        esp_rom_delay_us(1);
    }
    return count;
}

int dht11_init(int gpio_num)
{
    gpio_set_direction(gpio_num,GPIO_MODE_OUTPUT);
    if(gpio_set_level(gpio_num,0)!=ESP_OK)
    {
        return DHT22_INIT_ERROR;
    }
    esp_rom_delay_us(19000);
    if(gpio_set_level(gpio_num,1)!=ESP_OK)
    {
        return DHT22_INIT_ERROR;
    }
    esp_rom_delay_us(25);
    return INIT_OK;
}

int dht11_read(int gpio_num,float *humidity, float *temperature)
{
    uint8_t Data[5]={0};
    int8_t bit=7; 
    uint8_t reg=0;
    int timepresent;
    gpio_set_direction(gpio_num,GPIO_MODE_INPUT);
    timepresent=getSignalLevel(85,0,gpio_num);
    if(timepresent<0)
        return DHT22_TIMEOUT_ERROR;

    timepresent=getSignalLevel(85,1,gpio_num);
    if(timepresent<0) 
        return DHT22_TIMEOUT_ERROR;

    for (int i=0; i<40;i++)
    {
        timepresent=getSignalLevel(55, 0, gpio_num);
        if(timepresent <0)
            return DHT22_TIMEOUT_ERROR;
        timepresent= getSignalLevel(75, 1, gpio_num);
        if (timepresent < 40) 
            Data[reg] &= ~(1 << bit);
        else if (timepresent >= 40)
            Data[reg] |= (1 << bit);
        bit--;
        if (bit < 0)
        {
            bit = 7;
            reg++;
        }
    }
    if(humidity != NULL) 
    {
       *humidity= Data[0] +(float)Data[1] * 0.1f;
    }
    if(temperature!=NULL) 
    {
        *temperature = (Data[2] & 0x7F) + (float)Data[3]*0.1f;
        if(Data[2] & 0x80) *temperature *=-1.0;
    }
    if (Data[4] == ((Data[0] + Data[1] + Data[2] + Data[3]) & 0xFF)) 
        return DATA_OK;
    else 
        return DHT22_CHECKSUM_ERROR;
}

