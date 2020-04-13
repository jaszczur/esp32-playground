#ifndef HTTP_CONTROLLERS_H
#define HTTP_CONTROLLERS_H

#include "esp_http_server.h"
#include "esp_vfs.h"

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
  char resources_path[ESP_VFS_PATH_MAX + 1];
  char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

static const char *REST_TAG = "esp-rest";

esp_err_t app_http_get_readings(httpd_req_t *req);
esp_err_t app_http_post_light(httpd_req_t *req);

#endif /* HTTP_CONTROLLERS_H */
