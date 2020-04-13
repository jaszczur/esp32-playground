#include "sensors.h"
#include "dht11.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "time.h"
#include "moisture.h"
#include "luminescence.h"

void sensors_read(sensors_reading_t *reading) {
  dht11_reading_t temp_hum;

  temp_hum = DHT11_read();

  reading->moisture = moisture_read();
  reading->luminescence = luminescence_read();

  if (temp_hum.status == DHT11_OK) {
    reading->temperature = temp_hum.temperature;
    reading->humidity = temp_hum.humidity;
  } else {
    reading->temperature = -1;
    reading->humidity = -1;
  }
}

esp_err_t sensors_init(sensors_conf_t *conf) {
  gpio_set_direction(conf->dht11_pin, GPIO_MODE_INPUT);
  gpio_set_pull_mode(conf->dht11_pin, GPIO_PULLUP_ONLY);
  DHT11_init(conf->dht11_pin);

  moisture_init(conf->moisture_pin);

  return ESP_OK;
}
