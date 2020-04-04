#include "dht11_tasks.h"
#include "app_events.h"
#include "dht11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "dht11-task";

static void dht11_task(void *args) {
  dht11_reading_t reading;
  while (1) {
    reading = DHT11_read();
    if (reading.status == DHT11_OK) {
      // TODO - deal with this pointer
      app_publish_event(TEMP_HUM_READING, &reading, sizeof(dht11_reading_t),
                        portMAX_DELAY);
    } else {
      ESP_LOGW(TAG, "Got error from sensor: %d", reading.status);
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

esp_err_t dht11_start_read_loop(gpio_num_t pin) {
  DHT11_init(pin);
  xTaskCreate(dht11_task, "dht11_task", 2048, NULL, uxTaskPriorityGet(NULL),
              NULL);
  return 0;
}
