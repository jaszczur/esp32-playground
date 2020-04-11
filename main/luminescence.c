#include "luminescence.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"

static adc1_channel_t luminescence_channel;

int luminescence_read(void) {
  vTaskDelay(20 / portTICK_PERIOD_MS);
  return adc1_get_raw(luminescence_channel);
}

void luminescence_init(adc1_channel_t channel) {
  luminescence_channel = channel;
  adc1_config_channel_atten(luminescence_channel, ADC_ATTEN_DB_11);
}
