#include "app_events.h"
#include "app_mqtt.h"
#include "esp_sntp.h"
#include "dht11.h"
#include "dht11_tasks.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sntp.h"
#include "time.h"
#include "wifi_sta.h"

#define PIN_DHT11 GPIO_NUM_5
#define APP_MQTT_URL CONFIG_ESP_MQTT_URL

static const char *TAG = "app_main";

// TODO: atomic access and extract to file
static time_t last_reading_time;
static time_t last_publish_time;

static void on_network_connected(void *handler_args, esp_event_base_t base,
                                 int32_t evt_id, void *event_data) {
  if (base == APP_EVENTS && evt_id == APP_NETWORK_AVAILABLE) {
    ESP_LOGI(TAG, "Network is available. Connecting to MQTT broker");
    sntp_restart();
    ESP_ERROR_CHECK(app_mqtt_connect());
  } else {
    ESP_LOGW(TAG, "Got strange event... %d", evt_id);
  }
}

static void on_mqtt_disconnected(void *handler_args, esp_event_base_t base,
                                 int32_t evt_id, void *event_data) {
  /*
    As for now restarting is the only 100% working "solution".
    Other than that I achieved at least 6h of uptime with
    idf.py erase_flash + changes in loglevels in
    ${IDF_PATH}/components/lwip/port/esp32/include/lwipopts.h

    #define NETIF_DEBUG                     LWIP_DBG_ON
    #define IP_DEBUG                        LWIP_DBG_ON

    Without this it gets restarted every 10 or so minutes.
  */

  ESP_LOGI(TAG,
           "Disconnected from MQTT. Restart as of possible tcp/wifi stack bug");
  /* esp_restart(); */
  wifi_reconnect();
  /* vTaskDelay(1000 / portTICK_PERIOD_MS); */
  /* ESP_LOGI(TAG, "mqtt connect"); */
  /* app_mqtt_connect(); */
}

static void on_temp_hum_reading(void *handler_args, esp_event_base_t base,
                                int32_t evt_id, void *event_data) {

  ESP_LOGI(TAG, "Free heap size %d kB", xPortGetFreeHeapSize() / 1024);

  dht11_reading_t *reading = (dht11_reading_t *)event_data;
  ESP_LOGI(TAG, "Got reading: temp=%d degC, hum=%d%%, status=%d",
           reading->temperature, reading->humidity, reading->status);

  static const int buff_size = 5;
  char buff[buff_size];
  esp_err_t err;

  time(&last_reading_time);

  snprintf(buff, buff_size, "%d", reading->temperature);
  err = app_mqtt_publish(TOPIC_TEMPERATURE, buff, 0, 1, 0);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Could not send temperature reading");
  }

  snprintf(buff, buff_size, "%d", reading->humidity);
  err = app_mqtt_publish(TOPIC_HUMIDITY, buff, 0, 1, 0);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Could not send humidity reading");
  }
}

static void on_message_published(void *handler_args, esp_event_base_t base,
                                int32_t evt_id, void *event_data) {
  time(&last_publish_time);
}

static void mqtt_watchdog_task(void* task) {
  time_t time_diff;
  ESP_LOGI(TAG, "Starting message sending watchdog");

  while (true) {
    time_diff = last_reading_time - last_publish_time;
    ESP_LOGI(TAG, "Current diff time diff: %ld - %ld = %ld s", last_reading_time, last_publish_time, time_diff);
    if (time_diff > 30) {
      ESP_LOGW(TAG, "Not sending messages for %ld seconds", time_diff);
    }
    if (time_diff > 180) {
      ESP_LOGW(TAG, "Not sending messages for too long - restarting");
      esp_restart();
    }

    vTaskDelay(12000 / portTICK_PERIOD_MS);
  }
}

void app_main(void) {
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Initialize NVS
  /*
  ESP_ERROR_CHECK(nvs_flash_erase());
  ESP_ERROR_CHECK(nvs_flash_init());
  */
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize application event loop
  ESP_ERROR_CHECK(app_events_init());

  // Register application event handlers
  ESP_ERROR_CHECK(
      app_listen_for_event(APP_NETWORK_AVAILABLE, on_network_connected, NULL));
  ESP_ERROR_CHECK(
      app_listen_for_event(APP_TEMP_HUM_READING, on_temp_hum_reading, NULL));
  ESP_ERROR_CHECK(
      app_listen_for_event(APP_MESSAGE_PUBLISHED, on_message_published, NULL));

  // Setup temperature and humidity sensor
  ESP_ERROR_CHECK(dht11_start_read_loop(PIN_DHT11));

  // Initialize WiFi Station
  wifi_init_sta();

  // Configure and start SNTP time sync
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();

  // Start MQTT client
  static esp_mqtt_client_config_t mqtt_cfg = {
      .uri = APP_MQTT_URL,
      .disable_auto_reconnect = false,
  };
  app_mqtt_init(&mqtt_cfg);

  xTaskCreate(mqtt_watchdog_task, "app-mqtt-watchdog", 2048, NULL, tskIDLE_PRIORITY, NULL);
}
