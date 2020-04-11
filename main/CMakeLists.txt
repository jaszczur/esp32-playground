idf_component_register(
  SRCS
  "app_events.c"
  "wifi_sta.c"
  "sensor_tasks.c"
  "app_mqtt.c"
  "moisture.c"
  "luminescence.c"
  "relay.c"
  "main.c"

  REQUIRES
  dht11
  nvs_flash
  mqtt
  lwip

  INCLUDE_DIRS
  ".")
