#include "app_config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "nvs_flash.h"
#include <string.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include "ping/ping_sock.h"
#include "wifi_sta.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi-sta";

static int s_retry_num = 0;
static esp_ping_handle_t ping = NULL;

static void on_ping_end(esp_ping_handle_t hdl, void *args) {
  uint32_t transmitted;
  uint32_t received;
  uint32_t total_time_ms;

  esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted,
                       sizeof(transmitted));
  esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
  esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms,
                       sizeof(total_time_ms));
  ESP_LOGI(TAG, "%d packets transmitted, %d received, time %dms", transmitted,
         received, total_time_ms);

  if ((100 * received) / transmitted < 50) {
    ESP_LOGW(TAG, "Connection lost");
    esp_ping_delete_session(ping);
    ping = NULL;
    esp_wifi_disconnect();
  }
}

void initialize_ping_session(ip_addr_t *target_addr) {
  esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
  ping_config.target_addr = *target_addr;
  ping_config.count = 5;
  ping_config.interval_ms = 600;
  ping_config.timeout_ms = 600;

  /* set callback functions */
  esp_ping_callbacks_t cbs;
  cbs.on_ping_success = NULL;
  cbs.on_ping_timeout = NULL;
  cbs.on_ping_end = on_ping_end;
  cbs.cb_args = NULL;

  esp_ping_new_session(&ping_config, &cbs, &ping);
}

void wifi_check_connection() {
  if (ping == NULL) {
    ESP_LOGI(TAG, "Skipping check as not connected yet");
    return;
  }

  ESP_LOGI(TAG, "Pinging gateway to check connection");
  esp_ping_start(ping);
}



static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < APP_ESP_MAXIMUM_RETRY) {
      vTaskDelay(4000 / portTICK_PERIOD_MS);
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      s_retry_num = 0;
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    ip_addr_t ping_target_addr = IPADDR4_INIT(event->ip_info.gw.addr);
    initialize_ping_session(&ping_target_addr);
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  } else if (event_base == WIFI_EVENT) {
    ESP_LOGI(TAG, "Got WIFI event %d", event_id);
  } else if (event_base == IP_EVENT) {
    ESP_LOGI(TAG, "Got IP event %d", event_id);
  }
}

void wifi_init_sta(void) {
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta = {.ssid = APP_ESP_WIFI_SSID, .password = APP_ESP_WIFI_PASS},
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID: %s password: ***", APP_ESP_WIFI_SSID);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID: %s, password: %s",
             APP_ESP_WIFI_SSID, APP_ESP_WIFI_PASS);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }

}
