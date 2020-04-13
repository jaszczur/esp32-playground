#include "relay.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/gpio_types.h"
#include "nvs.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define RELAY_NVS_KEY_SIZE 12

static const char *TAG = "app_relay";
static app_relay_config_t relay_conf = {0, NULL, NULL, NULL, 8, 20};
static bool* relay_status = NULL;

static void relay_nvs_key(char *out, int id) {
  snprintf(out, RELAY_NVS_KEY_SIZE, "relay%d", id);
}

void app_relay_init(app_relay_config_t *config) {
  relay_conf.num_relays = config->num_relays;
  relay_conf.nvs_handle = config->nvs_handle;
  relay_conf.on_hour_start = config->on_hour_start;
  relay_conf.on_hour_stop = config->on_hour_stop;

  relay_conf.relay_gpio_mapping =
      calloc(relay_conf.num_relays, sizeof(gpio_num_t));
  memcpy(relay_conf.relay_gpio_mapping, config->relay_gpio_mapping,
         relay_conf.num_relays * sizeof(gpio_num_t));

  relay_conf.config = calloc(relay_conf.num_relays, sizeof(int));
  memcpy(relay_conf.config, config->config,
         relay_conf.num_relays * sizeof(int));

  relay_status = calloc(relay_conf.num_relays, sizeof(bool));

  esp_err_t err;
  int8_t initial_status;
  char nvs_key[RELAY_NVS_KEY_SIZE];
  for (int id = 0; id < relay_conf.num_relays; ++id) {
    gpio_num_t pin = relay_conf.relay_gpio_mapping[id];
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);

    relay_nvs_key(nvs_key, id);
    err = nvs_get_i8(relay_conf.nvs_handle, nvs_key, &initial_status);
    if (err == ESP_OK) {
      relay_status[id] = initial_status;
      ESP_LOGI(TAG, "Sate from NVS %d", initial_status);
    } else {
      relay_status[id] = false;
      ESP_LOGW(TAG, "Wasn't able to set saved relay state. %s", esp_err_to_name(err));
    }
    gpio_set_level(pin, relay_status[id]);
  }
}

void app_relay_clean() {
  if (relay_conf.num_relays == 0) {
    return;
  }

  relay_conf.num_relays = 0;

  free(relay_conf.relay_gpio_mapping);
  relay_conf.relay_gpio_mapping = NULL;

  free(relay_conf.config);
  relay_conf.config = NULL;

  free(relay_status);
  relay_status = NULL;
}

void app_relay_update() {
  time_t rawtime;
  time(&rawtime);
  struct tm *info = localtime(&rawtime);
  // TODO: support other time schedules and different time
  //       spans (like night only)
  bool turned_on_from_schedule = info->tm_hour >= relay_conf.on_hour_start
    && info->tm_hour < relay_conf.on_hour_stop;
  char nvs_key[RELAY_NVS_KEY_SIZE];

  for (int id = 0; id < relay_conf.num_relays; ++id) {
    gpio_num_t pin = relay_conf.relay_gpio_mapping[id];
    int turned_on = (relay_conf.config[id] == APP_RELAY_SCHEDULE)
      ? turned_on_from_schedule : relay_conf.config[id];

    relay_status[id] = turned_on;
    gpio_set_level(pin, turned_on);
    relay_nvs_key(nvs_key, id);
    nvs_set_i8(relay_conf.nvs_handle, nvs_key, turned_on);
    ESP_LOGI(TAG, "Set relay %d configured as %d to state %d", id, relay_conf.config[id], turned_on);
  }
  nvs_commit(relay_conf.nvs_handle);
}

void app_relay_set(int id, int config) {
  if (relay_conf.num_relays == 0 || id >= relay_conf.num_relays || id < 0) {
    return;
  }

  relay_conf.config[id] = config;
  gpio_set_level(relay_conf.relay_gpio_mapping[id], config);
}

int app_relay_get(int id) {
  if (relay_conf.num_relays == 0 || id >= relay_conf.num_relays || id < 0) {
    return APP_RELAY_ERR;
  }
  return relay_conf.config[id];
}

bool app_relay_turned_on(int id) {
  if (relay_conf.num_relays == 0 || id >= relay_conf.num_relays || id < 0) {
    return false;
  }

  return relay_status[id];
}
