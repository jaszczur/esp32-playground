#ifndef DHT11_TASKS_H_
#define DHT11_TASKS_H_

#include "esp_err.h"
#include "driver/gpio.h"

esp_err_t dht11_start_read_loop(gpio_num_t pin);

#endif
