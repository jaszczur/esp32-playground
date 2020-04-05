#include "app_events.h"
#include "app_mqtt.h"
#include "dht11.h"
#include "dht11_tasks.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_sta.h"

#define PIN_DHT11 GPIO_NUM_18

static const char *TAG = "app_main";

static void on_network_connected(void *handler_args, esp_event_base_t base,
                                 int32_t evt_id, void *event_data) {
  if (base == APP_EVENTS && evt_id == NETWORK_AVAILABLE) {
    ESP_LOGI(TAG, "Network is available");
    app_mqtt_init();
  } else {
    ESP_LOGW(TAG, "Got strange event...");
  }
}

static void on_temp_hum_reading(void *handler_args, esp_event_base_t base,
                                int32_t evt_id, void *event_data) {
  dht11_reading_t *reading = (dht11_reading_t *)event_data;
  ESP_LOGI(TAG, "Got reading: temp=%d degC, hum=%d%%, status=%d", reading->temperature,
           reading->humidity, reading->status);

  static const int buff_size = 10;
  char buff[buff_size];

  snprintf(buff, buff_size, "%d", reading->temperature);
  ESP_ERROR_CHECK_WITHOUT_ABORT(app_mqtt_publish(TOPIC_TEMPERATURE, buff, 0, 0, 0));

  snprintf(buff, buff_size, "%d", reading->humidity);
  ESP_ERROR_CHECK_WITHOUT_ABORT(app_mqtt_publish(TOPIC_HUMIDITY, buff, 0, 0, 0));
}

void app_main() {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(app_events_init());

  ESP_ERROR_CHECK(
      app_listen_for_event(NETWORK_AVAILABLE, on_network_connected, NULL));
  ESP_ERROR_CHECK(
      app_listen_for_event(TEMP_HUM_READING, on_temp_hum_reading, NULL));

  gpio_set_direction(PIN_DHT11, GPIO_MODE_INPUT);
  gpio_set_pull_mode(PIN_DHT11, GPIO_PULLUP_ONLY);
  ESP_ERROR_CHECK(dht11_start_read_loop(PIN_DHT11));

  wifi_init_sta();
}
