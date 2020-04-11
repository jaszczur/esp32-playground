#ifndef RELAY_H
#define RELAY_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "hal/gpio_types.h"

typedef struct app_relay_config {
  int num_relays;
  gpio_num_t *relay_gpio_mapping;
  bool *status;
} app_relay_config_t;

void app_relay_init(app_relay_config_t *config);
void app_relay_clean();
void app_relay_turn(int id, bool turnedOn);
bool app_relay_turnedOn(int id);


#endif /* RELAY_H */
