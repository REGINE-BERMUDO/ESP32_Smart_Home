#include "app_web.h"

#include "esp_http_server.h"
#include "esp_log.h"

// Label this module's messages.
static const char *TAG = "gateway_web";

// Keep the server handle so we can avoid starting it twice.
static httpd_handle_t server = nullptr;

// Respond when a browser requests the home page.
static esp_err_t homeHandler(httpd_req_t *request)
{
        const char *page =
        "<!DOCTYPE html>"
        "<html><head>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>ESP32 Gateway</title>"
        "</head><body>"
        "<h1>ESP32 Smart Home</h1>"
        "<p>Relay controls</p>"
        "<form action='/on' method='post'>"
        "<button type='submit'>ON</button>"
        "</form>"
        "<form action='/off' method='post'>"
        "<button type='submit'>OFF</button>"
        "</form>"
        "<p>Mesh control is not connected yet.</p>"
        "</body></html>";

    httpd_resp_set_type(request, "text/html");
    return httpd_resp_send(request, page, HTTPD_RESP_USE_STRLEN);
}   

// Record an ON request without controlling the relay yet.
static esp_err_t onHandler(httpd_req_t *request)
{
    ESP_LOGI(TAG, "Web button requested ON");

    return httpd_resp_send(
        request,
        "ON request received. Mesh control is not connected yet.",
        HTTPD_RESP_USE_STRLEN);
}

// Record an OFF request without controlling the relay yet.
static esp_err_t offHandler(httpd_req_t *request)
{
    ESP_LOGI(TAG, "Web button requested OFF");

    return httpd_resp_send(
        request,
        "OFF request received. Mesh control is not connected yet.",
        HTTPD_RESP_USE_STRLEN);
}

// Start the server and register its home page.
void setupWeb(void)
{
    // Do nothing if the server is already running.
    if (server != nullptr)
    {
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    // Route requests for "/" to our home-page handler.
    httpd_uri_t home = {};
    home.uri = "/";
    home.method = HTTP_GET;
    home.handler = homeHandler;

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &home));
    
    // Handle POST requests from the ON button.
    httpd_uri_t onRoute = {};
    onRoute.uri = "/on";
    onRoute.method = HTTP_POST;
    onRoute.handler = onHandler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &onRoute));

    // Handle POST requests from the OFF button.
    httpd_uri_t offRoute = {};
    offRoute.uri = "/off";
    offRoute.method = HTTP_POST;
    offRoute.handler = offHandler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &offRoute));
    ESP_LOGI(TAG, "Web server started");
}