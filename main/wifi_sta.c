/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <string.h>

#include "lwip/err.h"
#include "lwip/sys.h"

#include "app_events.h"

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static const char *TAG = "app_wifi";

static int s_retry_num = 0;
static esp_netif_t *netif = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    unsigned long delay_ms = MIN(60000, s_retry_num * s_retry_num * 100);
    ESP_LOGI(TAG, "Retry #%d to connect to the AP in %ld ms", s_retry_num, delay_ms);
    vTaskDelay(delay_ms/ portTICK_PERIOD_MS);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
    s_retry_num++;
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got ip: " IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    app_publish_event(APP_NETWORK_AVAILABLE, NULL, 0, portMAX_DELAY);
  } else {
    ESP_LOGI(TAG, "Got network event %s: %d", event_base, event_id);
  }
}

static void wifi_register_event_handlers(void) {
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
}

static void netif_reconfigure(void) {
  if (netif != NULL) {
    esp_netif_destroy(netif);
  }
  netif = esp_netif_create_default_wifi_sta();
}

static void wifi_connect(void) {
  static wifi_config_t wifi_config = {
      .sta = {.ssid = EXAMPLE_ESP_WIFI_SSID, .password = EXAMPLE_ESP_WIFI_PASS},
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_reconnect(void) {
  ESP_LOGI(TAG, "Reconnecting to WiFi");
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());
  wifi_connect();
}

void wifi_init_sta(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  netif_reconfigure();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_register_event_handlers();
  wifi_connect();

  ESP_LOGI(TAG, "Initialized WiFi station");
}
