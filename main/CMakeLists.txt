idf_component_register(
  SRCS
  "wifi_sta.c"
  "sensors.c"
  "http_server.c"
  "http_controllers.c"
  "core.c"
  "moisture.c"
  "luminescence.c"
  "relay.c"
  "main.c"

  REQUIRES
  dht11
  nvs_flash
  spiffs
  esp_http_server
  json

  INCLUDE_DIRS
  ".")

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../frontend")

if(EXISTS ${WEB_SRC_DIR}/build)
  spiffs_create_partition_image(www ${WEB_SRC_DIR}/build FLASH_IN_PROJECT)
else()
  message(FATAL_ERROR "${WEB_SRC_DIR}/build doesn't exit. Please run 'yarn build' in ${WEB_SRC_DIR}")
endif()
