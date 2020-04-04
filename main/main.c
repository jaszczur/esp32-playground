#include "app_events.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_sta.h"

static const char* TAG = "app_main";

static void on_network_connected(void *handler_args, esp_event_base_t base,
                                 int32_t evt_id, void *event_data) {
  if (base == APP_EVENTS && evt_id == NETWORK_AVAILABLE) {
    ESP_LOGI(TAG, "Network is available");
  } else {
    ESP_LOGW(TAG, "Got strange event...");
  }
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
  ESP_ERROR_CHECK(app_listen_for_event(NETWORK_AVAILABLE, on_network_connected, NULL));
  wifi_init_sta();
}
