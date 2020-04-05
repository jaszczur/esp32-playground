#include "esp_log.h"

#include "app_events.h"
#include "app_mqtt.h"

static const char *TAG = "app_mqtt";
static esp_mqtt_client_handle_t app_mqtt_client = NULL;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
  esp_mqtt_client_handle_t client = event->client;
  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    app_publish_event(MQTT_CONNECTED, NULL, 0, 1000 / portTICK_PERIOD_MS);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
    app_publish_event(MQTT_DISCONNECTED, NULL, 0, 1000 / portTICK_PERIOD_MS);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Reconnecting");
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mqtt_client_reconnect(client));
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
    ESP_LOGI(TAG, "Error type: %d", event->error_handle->error_type);
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
  int msg_id = esp_mqtt_client_publish(app_mqtt_client, app_topic_names[topic],
                                       data, len, qos, retain);
  return (msg_id < 0) ? ESP_FAIL : ESP_OK;
}

esp_err_t app_mqtt_init(const esp_mqtt_client_config_t *mqtt_cfg) {
  app_topic_names[TOPIC_TEMPERATURE] = "sensors/esp32-1/temperature";
  app_topic_names[TOPIC_HUMIDITY] = "sensors/esp32-1/humidity";

  esp_err_t err;

  esp_mqtt_client_handle_t client = esp_mqtt_client_init(mqtt_cfg);

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
