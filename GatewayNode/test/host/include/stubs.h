#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_FAIL = -1, ESP_ERR_NO_MEM = 1;
#define ESP_ERROR_CHECK(result) assert((result) == ESP_OK)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
using httpd_handle_t = void *;
struct httpd_req_t { httpd_handle_t handle; int socket; };
struct httpd_config_t { bool lru_purge_enable; unsigned send_wait_timeout; };
struct httpd_uri_t { const char *uri; int method; esp_err_t (*handler)(httpd_req_t *); };
#define HTTPD_DEFAULT_CONFIG() httpd_config_t{}
constexpr int HTTP_GET = 0, HTTP_POST = 1, HTTPD_RESP_USE_STRLEN = -1;
esp_err_t httpd_resp_set_type(httpd_req_t *, const char *);
esp_err_t httpd_resp_set_hdr(httpd_req_t *, const char *, const char *);
esp_err_t httpd_resp_set_status(httpd_req_t *, const char *);
esp_err_t httpd_resp_send(httpd_req_t *, const char *, std::ptrdiff_t);
esp_err_t httpd_resp_send_chunk(httpd_req_t *, const char *, std::ptrdiff_t);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *, const char *, char *, size_t);
esp_err_t httpd_req_async_handler_begin(httpd_req_t *, httpd_req_t **);
esp_err_t httpd_req_async_handler_complete(httpd_req_t *);
int httpd_req_to_sockfd(httpd_req_t *);
esp_err_t httpd_sess_trigger_close(httpd_handle_t, int);
esp_err_t httpd_start(httpd_handle_t *, const httpd_config_t *);
esp_err_t httpd_register_uri_handler(httpd_handle_t, const httpd_uri_t *);
bool getRelayReportedState(bool *);
esp_err_t sendRelayCommand(bool);
using TickType_t = uint32_t;
using SemaphoreHandle_t = int *;
constexpr int pdPASS = 1, pdTRUE = 1;
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
SemaphoreHandle_t xSemaphoreCreateCounting(unsigned, unsigned);
int xSemaphoreTake(SemaphoreHandle_t, unsigned);
void xSemaphoreGive(SemaphoreHandle_t);
TickType_t xTaskGetTickCount();
void vTaskDelay(TickType_t);
void vTaskDelete(void *);
int xTaskCreate(void (*)(void *), const char *, unsigned, void *, unsigned, void *);
