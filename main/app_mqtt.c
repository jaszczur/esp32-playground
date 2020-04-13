#include "esp_err.h"
#include "esp_log.h"

#include "app_events.h"
#include "app_mqtt.h"
#include "freertos/portmacro.h"
#include "mqtt_client.h"
#include "relay.h"

static const char *TAG = "app_mqtt";
static esp_mqtt_client_handle_t app_mqtt_client = NULL;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
  switch (event->event_id) {

  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "Connected to MQTT broker. Client %#08x",
             (unsigned int)app_mqtt_client);

    // TODO Un-hardcode this
    esp_mqtt_client_subscribe(app_mqtt_client, app_topic_names[TOPIC_LIGHT_SET],
                              2);
    break;

  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGW(TAG, "Disconnected from MQTT broker");
    break;

  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    app_publish_event(APP_MESSAGE_PUBLISHED, &event->msg_id, sizeof(event->msg_id), portMAX_DELAY);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);

    // TODO: Un-hardcode this
    if(strncmp(app_topic_names[TOPIC_LIGHT_SET], event->topic, event->topic_len) == 0 && event->data_len > 0) {
      int relay_conf = event->data[0] - '0';
      app_relay_set(0, relay_conf);
    }
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
    ESP_LOGI(TAG, "Start failed with %s. We are already connected", esp_err_to_name(err));
    /* err = esp_mqtt_client_reconnect(app_mqtt_client); */
  }
  return ESP_OK;
}

esp_err_t app_mqtt_init(const esp_mqtt_client_config_t *mqtt_cfg) {
  ESP_LOGI(TAG, "Initializing MQTT connection");
  app_topic_names[TOPIC_TEMPERATURE] = "sensors/esp32-1/temperature";
  app_topic_names[TOPIC_HUMIDITY] = "sensors/esp32-1/humidity";
  app_topic_names[TOPIC_MOISTURE] = "sensors/esp32-1/moisture";
  app_topic_names[TOPIC_LUMINESCENCE] = "sensors/esp32-1/luminescence";
  app_topic_names[TOPIC_LIGHT_SET] = "sensors/esp32-1/light-set";
  app_topic_names[TOPIC_LIGHT_GET] = "sensors/esp32-1/light";

  app_mqtt_client = esp_mqtt_client_init(mqtt_cfg);

  return esp_mqtt_client_register_event(app_mqtt_client, ESP_EVENT_ANY_ID,
                                       mqtt_event_handler, app_mqtt_client);
}
