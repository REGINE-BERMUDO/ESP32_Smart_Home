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
        "<html><head><title>ESP32 Gateway</title></head>"
        "<body><h1>ESP32 Smart Home</h1>"
        "<p>Gateway web server is running.</p>"
        "</body></html>";

    httpd_resp_set_type(request, "text/html");
    return httpd_resp_send(request, page, HTTPD_RESP_USE_STRLEN);
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
    ESP_LOGI(TAG, "Web server started");
}