idf_component_register(
  SRCS "app_events.c" "wifi_sta.c" "dht11_tasks.c" "main.c"
  REQUIRES dht11 nvs_flash
  INCLUDE_DIRS ".")
