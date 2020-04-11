#include "moisture.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static adc1_channel_t moisture_channel;

int moisture_read(void) {
  vTaskDelay(20 / portTICK_PERIOD_MS);
  return adc1_get_raw(moisture_channel);
}

void moisture_init(adc1_channel_t channel) {
  moisture_channel = channel;
  adc1_config_channel_atten(moisture_channel, ADC_ATTEN_DB_6);
}
