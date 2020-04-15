#include "core.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "relay.h"
#include "wifi_sta.h"

static const int REFRESH_RATE_SEC = 30;
static const int LOOPS_TILL_RESET = 4 /* hours */ * 3600 / REFRESH_RATE_SEC;

static const char *TAG = "app_core";

void core_loop(void *params) {
  ESP_LOGI(TAG, "Started application core loop");
  for (int i = LOOPS_TILL_RESET; i >= 0; --i) {
    app_relay_update();
    wifi_check_connection();

    ESP_LOGI(TAG, "Loops till restart %d", i);
    vTaskDelay(REFRESH_RATE_SEC * 1000 / portTICK_PERIOD_MS);
  }

  ESP_LOGI(TAG, "Resetting as a http unresponsiveness fix");
  esp_restart();
  vTaskDelete(NULL);
}

esp_err_t app_core_loop_start() {
  BaseType_t ret = xTaskCreate(core_loop, TAG, 4096, NULL, 1, NULL);
  return ret == pdPASS ? ESP_OK : ESP_FAIL;
}
