#include "esp_log.h"
#include "mqtt_client.h"

#include "app_events.h"
#include "app_mqtt.h"

static const char *TAG = "app_mqtt";
static esp_mqtt_client_handle_t app_mqtt_client = NULL;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;
  // your_context_t *context = event->context;
  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    app_publish_event(MQTT_CONNECTED, NULL, 0, portMAX_DELAY);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED - reconnect");
    app_publish_event(MQTT_DISCONNECTED, NULL, 0, portMAX_DELAY);
    esp_err_t err = esp_mqtt_client_reconnect(client);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Reconnected");
    } else {
      ESP_LOGW(TAG, "Reconnection failed: %s", esp_err_to_name(err));
    }
    break;

  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    break;
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
  return ESP_OK;
}
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
           event_id);
  mqtt_event_handler_cb(event_data);
}

esp_err_t app_mqtt_publish(app_mqtt_topic_t topic, const char *data, int len,
                           int qos, int retain) {
  if (app_mqtt_client == NULL) {
    return ESP_ERR_INVALID_STATE;
  }
  return esp_mqtt_client_publish(app_mqtt_client, app_topic_names[topic], data,
                                 len, qos, retain);
}

esp_err_t app_mqtt_init() {
  app_topic_names[TOPIC_TEMPERATURE] = "sensors/esp32-1/temperature";
  app_topic_names[TOPIC_HUMIDITY] = "sensors/esp32-1/humidity";

  esp_err_t err;

  const esp_mqtt_client_config_t mqtt_cfg = {
      .uri = "mqtt://192.168.1.4",
  };
  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

  err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                       mqtt_event_handler, client);
  if (err != ESP_OK) {
    return err;
  }

  err = esp_mqtt_client_start(client);
  if (err != ESP_OK) {
    return err;
  }

  app_mqtt_client = client;
  return ESP_OK;
}
