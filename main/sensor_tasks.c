#include "sensor_tasks.h"
#include "app_events.h"
#include "dht11.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "time.h"
#include "moisture.h"
#include "luminescence.h"

static const char *TAG = "dht11-task";

static void sensors_task(void *args) {
  dht11_reading_t temp_hum;
  sensors_reading_t reading;

  while (1) {
    temp_hum = DHT11_read();

    reading.moisture = moisture_read();
    reading.luminescence = luminescence_read();

    if (temp_hum.status == DHT11_OK) {
      reading.temperature = temp_hum.temperature;
      reading.humidity = temp_hum.humidity;
    } else {
      reading.temperature = -1;
      reading.humidity = -1;
    }

    app_publish_event(APP_TEMP_HUM_READING, &reading, sizeof(sensors_reading_t),
                      portMAX_DELAY);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

esp_err_t sensors_start_loop(sensors_conf_t *conf) {
  gpio_set_direction(conf->dht11_pin, GPIO_MODE_INPUT);
  gpio_set_pull_mode(conf->dht11_pin, GPIO_PULLUP_ONLY);
  DHT11_init(conf->dht11_pin);

  moisture_init(conf->moisture_pin);

  xTaskCreate(sensors_task, TAG, 2048, NULL, tskIDLE_PRIORITY + 3, NULL);

  return ESP_OK;
}
