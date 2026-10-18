#include "app_web.h"
#include "app_ble.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <cstring>
#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "gateway_web";
static httpd_handle_t server = nullptr;
static SemaphoreHandle_t streamSlots = nullptr;

// Assets are embedded in flash by src/CMakeLists.txt; no filesystem is needed.
extern const unsigned char pageStart[] asm("_binary_index_html_start");
extern const unsigned char pageEnd[] asm("_binary_index_html_end");
extern const unsigned char scriptStart[] asm("_binary_app_js_start");
extern const unsigned char scriptEnd[] asm("_binary_app_js_end");
extern const unsigned char fontStart[] asm("_binary_inter_latin_woff2_start");
extern const unsigned char fontEnd[] asm("_binary_inter_latin_woff2_end");
extern const unsigned char licenseStart[] asm("_binary_licenses_txt_start");
extern const unsigned char licenseEnd[] asm("_binary_licenses_txt_end");

static esp_err_t homeHandler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, reinterpret_cast<const char *>(pageStart), pageEnd - pageStart);
}

static esp_err_t scriptHandler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "application/javascript; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, reinterpret_cast<const char *>(scriptStart), scriptEnd - scriptStart);
}

static esp_err_t fontHandler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "font/woff2");
    httpd_resp_set_hdr(request, "Cache-Control", "public, max-age=86400");
    return httpd_resp_send(request, reinterpret_cast<const char *>(fontStart), fontEnd - fontStart);
}

static esp_err_t licenseHandler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "text/plain; charset=utf-8");
    return httpd_resp_send(request, reinterpret_cast<const char *>(licenseStart), licenseEnd - licenseStart);
}

static esp_err_t handleCommand(httpd_req_t *request, bool on)
{
    const esp_err_t result = sendRelayCommand(on);
    if (result == ESP_OK)
    {
        ESP_LOGI(TAG, "Web %s command accepted for sending", on ? "ON" : "OFF");
    }
    else
    {
        ESP_LOGE(TAG, "%s send failed: %s", on ? "ON" : "OFF", esp_err_to_name(result));
    }

    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    char requestedWith[16] = {};
    const bool async = httpd_req_get_hdr_value_str(
        request, "X-Requested-With", requestedWith, sizeof(requestedWith)) == ESP_OK &&
        std::strcmp(requestedWith, "fetch") == 0;

    // Ordinary form submissions also return to the dashboard, without a Back step.
    if (!async)
    {
        httpd_resp_set_status(request, "303 See Other");
        httpd_resp_set_hdr(request, "Location", "/");
        return httpd_resp_send(request, nullptr, 0);
    }

    httpd_resp_set_type(request, "text/plain; charset=utf-8");
    if (result != ESP_OK)
    {
        httpd_resp_set_status(request, "503 Service Unavailable");
        return httpd_resp_send(request,
            "Check the gateway's Mesh provisioning and Lights key binding, then try again.",
            HTTPD_RESP_USE_STRLEN);
    }
    return httpd_resp_send(request, "Request sent. Relay state is not confirmed.", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t onHandler(httpd_req_t *request)
{
    return handleCommand(request, true);
}

static esp_err_t statusHandler(httpd_req_t *request)
{
    bool on = false;
    const bool known = getRelayReportedState(&on);
    const char *body = !known ? "{\"state\":\"unknown\"}" :
                       on ? "{\"state\":\"on\"}" : "{\"state\":\"off\"}";
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, body, HTTPD_RESP_USE_STRLEN);
}

// Async workers leave the HTTP server task free to serve ON/OFF and other assets.
static void stateStreamTask(void *argument)
{
    httpd_req_t *request = static_cast<httpd_req_t *>(argument);
    httpd_resp_set_type(request, "text/event-stream");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    int previousState = -2;
    TickType_t sentAt = xTaskGetTickCount();
    while (true)
    {
        bool on = false;
        const bool known = getRelayReportedState(&on);
        const int state = known ? (on ? 1 : 0) : -1;
        const TickType_t now = xTaskGetTickCount();
        if (state != previousState || now - sentAt >= pdMS_TO_TICKS(2000))
        {
            char event[64];
            const int length = std::snprintf(event, sizeof(event),
                "data: {\"state\":\"%s\"}\n\n", !known ? "unknown" : on ? "on" : "off");
            if (httpd_resp_send_chunk(request, event, length) != ESP_OK) break;
            previousState = state;
            sentAt = now;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    const int socket = httpd_req_to_sockfd(request);
    httpd_handle_t handle = request->handle;
    httpd_req_async_handler_complete(request);
    httpd_sess_trigger_close(handle, socket);
    xSemaphoreGive(streamSlots);
    vTaskDelete(nullptr);
}

static esp_err_t eventsHandler(httpd_req_t *request)
{
    // Bound memory/socket use; extra browser tabs continue using /status polling.
    if (xSemaphoreTake(streamSlots, 0) != pdTRUE)
    {
        httpd_resp_set_status(request, "503 Service Unavailable");
        return httpd_resp_send(request, "Live updates are busy; polling remains available.", HTTPD_RESP_USE_STRLEN);
    }
    httpd_req_t *asyncRequest = nullptr;
    const esp_err_t result = httpd_req_async_handler_begin(request, &asyncRequest);
    if (result != ESP_OK)
    {
        xSemaphoreGive(streamSlots);
        return result;
    }
    if (xTaskCreate(stateStreamTask, "relay_events", 4096, asyncRequest, 5, nullptr) != pdPASS)
    {
        httpd_resp_set_status(asyncRequest, "503 Service Unavailable");
        httpd_resp_send(asyncRequest, "Live updates unavailable.", HTTPD_RESP_USE_STRLEN);
        httpd_req_async_handler_complete(asyncRequest);
        xSemaphoreGive(streamSlots);
    }
    return ESP_OK;
}

static esp_err_t offHandler(httpd_req_t *request)
{
    return handleCommand(request, false);
}

void setupWeb(void)
{
    if (server != nullptr) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.send_wait_timeout = 2;
    streamSlots = xSemaphoreCreateCounting(2, 2);
    ESP_ERROR_CHECK(streamSlots != nullptr ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = homeHandler},
        {.uri = "/app.js", .method = HTTP_GET, .handler = scriptHandler},
        {.uri = "/inter-latin.woff2", .method = HTTP_GET, .handler = fontHandler},
        {.uri = "/licenses.txt", .method = HTTP_GET, .handler = licenseHandler},
        {.uri = "/on", .method = HTTP_POST, .handler = onHandler},
        {.uri = "/off", .method = HTTP_POST, .handler = offHandler},
        {.uri = "/status", .method = HTTP_GET, .handler = statusHandler},
        {.uri = "/events", .method = HTTP_GET, .handler = eventsHandler},
    };
    for (const auto &route : routes)
    {
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &route));
    }
    ESP_LOGI(TAG, "Web server started");
}
