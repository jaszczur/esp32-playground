#include "relay.h"
#include "hal/gpio_types.h"
#include "string.h"
#include "driver/gpio.h"

static app_relay_config_t relay_state = {0, NULL, NULL};

void app_relay_init(app_relay_config_t *config) {
  relay_state.num_relays = config->num_relays;

  relay_state.relay_gpio_mapping =
      calloc(relay_state.num_relays, sizeof(gpio_num_t));
  memcpy(relay_state.relay_gpio_mapping, config->relay_gpio_mapping,
         relay_state.num_relays * sizeof(gpio_num_t));

  relay_state.status = calloc(relay_state.num_relays, sizeof(bool));
  memcpy(relay_state.status, config->relay_gpio_mapping,
         relay_state.num_relays * sizeof(gpio_num_t));

  for(int id = 0; id<relay_state.num_relays; ++id) {
    gpio_num_t pin = relay_state.relay_gpio_mapping[id];
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, relay_state.status[id] ? 1 : 0);
  }
}

void app_relay_clean() {
  if (relay_state.num_relays == 0) {
    return;
  }

  relay_state.num_relays = 0;
  free(relay_state.relay_gpio_mapping);
  free(relay_state.status);
}

void app_relay_turn(int id, bool turnedOn) {
  if (relay_state.num_relays == 0 || id >= relay_state.num_relays || id < 0) {
    return;
  }

  relay_state.status[id] = turnedOn;
  gpio_set_level(relay_state.relay_gpio_mapping[id], turnedOn ? 1 : 0);
}

bool app_relay_turnedOn(int id) {
  if (relay_state.num_relays == 0 || id >= relay_state.num_relays || id < 0) {
    return false;
  }
  return relay_state.status[id];
}
