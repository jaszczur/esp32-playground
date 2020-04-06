#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_events.h"
#include "esp_event_base.h"

static const char *TAG = "app_event_loop";

// Event loops
esp_event_loop_handle_t loop_with_task;

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(APP_EVENTS);

esp_err_t app_events_init(void) {
  ESP_LOGI(TAG, "setting up");

  esp_event_loop_args_t loop_with_task_args = {
      .queue_size = 16,
      .task_name = "loop_task", // task will be created
      .task_priority = uxTaskPriorityGet(NULL) + 1,
      .task_stack_size = 9 * 1024,
      .task_core_id = tskNO_AFFINITY};

  return esp_event_loop_create(&loop_with_task_args, &loop_with_task);
}

esp_err_t app_listen_for_event(app_evt_t evt,
                               esp_event_handler_t app_evt_handler,
                               void *event_handler_data) {
  ESP_LOGI(TAG, "Registering handler for event %d", evt);
  return esp_event_handler_register_with(loop_with_task, APP_EVENTS, evt,
                                         app_evt_handler, event_handler_data);
}

esp_err_t app_publish_event(app_evt_t evt, void *event_data,
                            size_t event_data_size, TickType_t ticks_to_wait) {
  ESP_LOGI(TAG, "Posting app event %d", evt);
  return esp_event_post_to(loop_with_task, APP_EVENTS, evt, event_data, event_data_size,ticks_to_wait);
}
