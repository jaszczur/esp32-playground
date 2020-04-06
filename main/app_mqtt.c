#include "esp_err.h"
#include "esp_log.h"

#include "app_events.h"
#include "app_mqtt.h"
#include "freertos/portmacro.h"
#include "mqtt_client.h"

static const char *TAG = "app_mqtt";
static esp_mqtt_client_handle_t app_mqtt_client = NULL;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
  switch (event->event_id) {

  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "Connected to MQTT broker. Client %#08x",
             (unsigned int)app_mqtt_client);
    app_publish_event(APP_MQTT_CONNECTED, NULL, 0, 1000 / portTICK_PERIOD_MS);
    break;

  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGW(TAG, "Disconnected from MQTT broker");
    app_publish_event(APP_MQTT_DISCONNECTED, NULL, 0, portMAX_DELAY);
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
    ESP_LOGI(TAG, "Other MQTT event id:%d", event->event_id);
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

esp_err_t app_mqtt_connect(void) {
  esp_err_t err = esp_mqtt_client_start(app_mqtt_client);
  if (err != ESP_OK) {
    // Maybe we should just reconnect?
    ESP_LOGI(TAG, "Start failed with %s", esp_err_to_name(err));
    err = esp_mqtt_client_reconnect(app_mqtt_client);
  }
  return err;
}

esp_err_t app_mqtt_init(const esp_mqtt_client_config_t *mqtt_cfg) {
  ESP_LOGI(TAG, "Initializing MQTT connection");
  app_topic_names[TOPIC_TEMPERATURE] = "sensors/esp32-1/temperature";
  app_topic_names[TOPIC_HUMIDITY] = "sensors/esp32-1/humidity";

  app_mqtt_client = esp_mqtt_client_init(mqtt_cfg);

  return esp_mqtt_client_register_event(app_mqtt_client, ESP_EVENT_ANY_ID,
                                       mqtt_event_handler, app_mqtt_client);
}