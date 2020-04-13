#ifndef RELAY_H
#define RELAY_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "hal/gpio_types.h"
#include "nvs.h"


#define APP_RELAY_SCHEDULE 2
#define APP_RELAY_ON 1
#define APP_RELAY_OFF 0
#define APP_RELAY_ERR -1

typedef struct app_relay_config {
  int num_relays;
  gpio_num_t *relay_gpio_mapping;
  int *config;
  nvs_handle_t nvs_handle;
  int on_hour_start;
  int on_hour_stop;
} app_relay_config_t;

void app_relay_init(app_relay_config_t *config);
void app_relay_clean();
void app_relay_update();
void app_relay_set(int id, int status);
int app_relay_get(int id);
bool app_relay_turned_on(int id);


#endif /* RELAY_H */
