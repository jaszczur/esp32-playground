idf_component_register(
  SRCS "app_events.c" "wifi_sta.c" "dht11_tasks.c" "app_mqtt.c" "main.c"
  REQUIRES dht11 nvs_flash mqtt lwip
  INCLUDE_DIRS ".")
