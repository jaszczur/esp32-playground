#include "core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "relay.h"

static const char *TAG = "app_core";

void core_loop(void *params) {
  while (true) {
    // Update lights
    app_relay_update();

    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

esp_err_t app_core_loop_start() {
  return xTaskCreate(core_loop, TAG, 2048, NULL, tskIDLE_PRIORITY, NULL);
}
