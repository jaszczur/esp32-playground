#include "core.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "relay.h"
#include "wifi_sta.h"

static const char *TAG = "app_core";

void core_loop(void *params) {
  ESP_LOGI(TAG, "Started application core loop");
  while (true) {
    // Update lights
    app_relay_update();

    wifi_check_connection();

    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

esp_err_t app_core_loop_start() {
  BaseType_t ret = xTaskCreate(core_loop, TAG, 4096, NULL, 1, NULL);
  return ret == pdPASS ? ESP_OK : ESP_FAIL;
}
