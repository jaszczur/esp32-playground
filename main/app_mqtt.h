#ifndef APP_MQTT_H
#define APP_MQTT_H

#include "esp_err.h"

typedef enum { TOPIC_TEMPERATURE, TOPIC_HUMIDITY, TOPIC_COUNT } app_mqtt_topic_t;

const char *app_topic_names[TOPIC_COUNT];

esp_err_t app_mqtt_init();
esp_err_t app_mqtt_publish(app_mqtt_topic_t topic, const char *data, int len,
                           int qos, int retain);

#endif /* APP_MQTT_H */
