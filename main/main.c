#include "app_events.h"
#include "app_mqtt.h"
#include "dht11.h"
#include "driver/adc_common.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "nvs_flash.h"
#include "relay.h"
#include "sensor_tasks.h"
#include "sntp.h"
#include "time.h"
#include "wifi_sta.h"

#define PIN_DHT11 GPIO_NUM_5
#define PIN_MOISTURE ADC1_CHANNEL_7
#define PIN_LUMINESCENCE ADC1_CHANNEL_0
#define PIN_RELAY_LIGHTS GPIO_NUM_32
#define PIN_STATUS_WARN GPIO_NUM_33
#define APP_MQTT_URL CONFIG_ESP_MQTT_URL

#define RELAY_LIGHT 0

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

static void publish_reading(const char *name, app_mqtt_topic_t topic, int value, char *buff, int buff_size) {
  snprintf(buff, buff_size, "%d", value);
  esp_err_t err = app_mqtt_publish(topic, buff, 0, 1, 0);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Could not send %s reading", name);
  }
}

static void on_sensors_reading(void *handler_args, esp_event_base_t base,
                               int32_t evt_id, void *event_data) {

  ESP_LOGI(TAG, "Free heap size %d kB", xPortGetFreeHeapSize() / 1024);

  sensors_reading_t *reading = (sensors_reading_t *)event_data;
  ESP_LOGI(TAG, "Got reading: temp=%d degC, hum=%d%%, moist=%d, luminescence=%d",
           reading->temperature, reading->humidity, reading->moisture, reading->luminescence);

  static const int buff_size = 8;
  char buff[buff_size];

  time(&last_reading_time);

  publish_reading("temperature", TOPIC_TEMPERATURE, reading->temperature, buff, buff_size);
  publish_reading("humidity", TOPIC_HUMIDITY, reading->humidity, buff, buff_size);
  publish_reading("moisture", TOPIC_MOISTURE, reading->moisture, buff, buff_size);
  publish_reading("luminescence", TOPIC_LUMINESCENCE, reading->luminescence, buff,
                  buff_size);
  publish_reading("light", TOPIC_LIGHT_GET, app_relay_turnedOn(RELAY_LIGHT), buff, buff_size);

  bool status_warn = reading->moisture < 2000;
  gpio_set_level(PIN_STATUS_WARN, status_warn);
}

static void on_message_published(void *handler_args, esp_event_base_t base,
                                 int32_t evt_id, void *event_data) {
  time(&last_publish_time);
}

static void mqtt_watchdog_task(void *task) {
  time_t time_diff;
  ESP_LOGI(TAG, "Starting message sending watchdog");

  while (true) {
    time_diff = labs(last_reading_time - last_publish_time);
    if (time_diff > 3600) {
      ESP_LOGI(TAG, "Invalid time - skiping check");
    } else {
    ESP_LOGI(TAG, "Current diff time diff: %ld - %ld = %ld s",
             last_reading_time, last_publish_time, time_diff);
    if (time_diff > 30) {
      ESP_LOGW(TAG, "Not sending messages for %ld seconds", time_diff);
    }
    if (time_diff > 180) {
      ESP_LOGW(TAG, "Not sending messages for too long - restarting");
      esp_restart();
    }
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

  // Initialize UI pins
  gpio_set_direction(PIN_STATUS_WARN, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_STATUS_WARN, 1);

  // Initialize relay
  gpio_num_t relay_pins[] = {PIN_RELAY_LIGHTS};
  int initial_relay_status[] = {1};
  app_relay_config_t app_relay_configuration =
    {
     .num_relays = 1,
     .relay_gpio_mapping = relay_pins,
     .status = initial_relay_status,
    };
  app_relay_init(&app_relay_configuration);

  // Register application event handlers
  ESP_ERROR_CHECK(
      app_listen_for_event(APP_NETWORK_AVAILABLE, on_network_connected, NULL));
  ESP_ERROR_CHECK(
      app_listen_for_event(APP_TEMP_HUM_READING, on_sensors_reading, NULL));
  ESP_ERROR_CHECK(
      app_listen_for_event(APP_MESSAGE_PUBLISHED, on_message_published, NULL));

  // Setup temperature and humidity sensor
  adc1_config_width(ADC_WIDTH_BIT_12);
  sensors_conf_t sensors_configuration = {
      .dht11_pin = PIN_DHT11,
      .moisture_pin = PIN_MOISTURE,
      .luminescence_pin = PIN_LUMINESCENCE,
  };
  ESP_ERROR_CHECK(sensors_start_loop(&sensors_configuration));

  // Initialize WiFi Station
  wifi_init_sta();

  // Configure timezone and start SNTP time sync
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();

  // Start MQTT client
  static esp_mqtt_client_config_t mqtt_cfg = {
      .uri = APP_MQTT_URL,
      .disable_auto_reconnect = false,
  };
  app_mqtt_init(&mqtt_cfg);

  xTaskCreate(mqtt_watchdog_task, "app-mqtt-watchdog", 2048, NULL,
              tskIDLE_PRIORITY, NULL);
}
