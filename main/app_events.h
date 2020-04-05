#ifndef APP_EVENTS_H_
#define APP_EVENTS_H_

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_timer.h"

// Declarations for the event source
#define TASK_ITERATIONS_COUNT 10 // number of times the task iterates
#define TASK_PERIOD 500          // period of the task loop in milliseconds

ESP_EVENT_DECLARE_BASE(APP_EVENTS); // declaration of the task events family

typedef enum {
              NETWORK_AVAILABLE, // raised when nework is connected
              TEMP_HUM_READING,
              MQTT_CONNECTED,
              MQTT_DISCONNECTED
} app_evt_t;

esp_err_t app_events_init(void);

esp_err_t app_listen_for_event(app_evt_t evt,
                               esp_event_handler_t app_evt_handler,
                               void *event_handler_data);

esp_err_t app_publish_event(app_evt_t evt, void *event_data,
                            size_t event_data_size, TickType_t ticks_to_wait);

#endif
