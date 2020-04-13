#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_err.h"

esp_err_t app_httpd_init(const char *base_path);
esp_err_t app_httpd_start();

#endif /* HTTP_SERVER_H */
