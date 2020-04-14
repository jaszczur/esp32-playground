#include "http_controllers.h"
#include "app_config.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "relay.h"
#include "sensors.h"
#include "esp_log.h"
#include <time.h>

esp_err_t app_http_get_readings(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  cJSON *root = cJSON_CreateObject();

  sensors_reading_t reading;
  sensors_read(&reading);
  time_t rawtime;
  time(&rawtime);

  cJSON_AddNumberToObject(root, "ts", rawtime);
  cJSON_AddNumberToObject(root, "temperature", reading.temperature);
  cJSON_AddNumberToObject(root, "humidity", reading.humidity);
  cJSON_AddNumberToObject(root, "luminescence", reading.luminescence);
  cJSON_AddNumberToObject(root, "moisture", reading.moisture);
  cJSON_AddNumberToObject(root, "light", app_relay_turned_on(RELAY_LIGHT));
  cJSON_AddNumberToObject(root, "light-conf", app_relay_get(RELAY_LIGHT));

  const char *json_as_string = cJSON_PrintUnformatted(root);
  httpd_resp_sendstr(req, json_as_string);
  free((void *)json_as_string);
  cJSON_Delete(root);

  ESP_LOGI(REST_TAG, "Sent sensor data");
  return ESP_OK;
}

esp_err_t app_http_post_light(httpd_req_t *req) {
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int new_status = cJSON_GetObjectItem(root, "status")->valueint;
    cJSON_Delete(root);

    app_relay_set(RELAY_LIGHT, new_status);
    app_relay_update();

    httpd_resp_sendstr(req, "{}");
    ESP_LOGI(REST_TAG, "Changed light state");

    return ESP_OK;
}
