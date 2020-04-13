#include "app_config.h"
#include "core.h"
#include "dht11.h"
#include "driver/adc_common.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_vfs_semihost.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "http_server.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "relay.h"
#include "sensors.h"
#include "sntp.h"
#include "time.h"
#include "wifi_sta.h"

static const char *TAG = "app_main";
static nvs_handle_t app_nvs_handle;

esp_err_t init_fs(void) {
  esp_vfs_spiffs_conf_t conf = {.base_path = "/www",
                                .partition_label = NULL,
                                .max_files = 5,
                                .format_if_mount_failed = false};
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
             esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
  return ESP_OK;
}

void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_ERROR_CHECK(nvs_open("sensapp", NVS_READWRITE, &app_nvs_handle));

  // Initialize WiFi Station
  wifi_init_sta();

  // Configure timezone and start SNTP time sync
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();

  // Setup temperature and humidity sensor
  adc1_config_width(ADC_WIDTH_BIT_12);
  sensors_conf_t sensors_configuration = {
      .dht11_pin = PIN_DHT11,
      .moisture_pin = PIN_MOISTURE,
      .luminescence_pin = PIN_LUMINESCENCE,
  };
  ESP_ERROR_CHECK(sensors_init(&sensors_configuration));

  // Start core loop
  ESP_ERROR_CHECK(app_core_loop_start());

  // Initialize relay
  gpio_num_t relay_pins[] = {PIN_RELAY_LIGHTS};
  int relay_config[] = {APP_RELAY_SCHEDULE};
  app_relay_config_t app_relay_configuration = {
      .num_relays = 1,
      .relay_gpio_mapping = relay_pins,
      .config = relay_config,
      .on_hour_start = 8,
      .on_hour_stop = 21,
      .nvs_handle = app_nvs_handle,
  };
  app_relay_init(&app_relay_configuration);

  // Initialize web resources
  init_fs();

  // Init HTTP Server
  ESP_ERROR_CHECK(app_httpd_init("/www"));
}
