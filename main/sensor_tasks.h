#ifndef DHT11_TASKS_H_
#define DHT11_TASKS_H_

#include "driver/adc.h"
#include "driver/adc_common.h"
#include "driver/gpio.h"
#include "esp_err.h"

typedef struct sensors_conf {
  gpio_num_t dht11_pin;
  adc1_channel_t moisture_pin;
  adc1_channel_t luminescence_pin;
} sensors_conf_t;

typedef struct sensors_reading {
  int temperature;
  int humidity;
  int moisture;
  int luminescence;
} sensors_reading_t;

esp_err_t sensors_start_loop(sensors_conf_t *conf);

#endif
