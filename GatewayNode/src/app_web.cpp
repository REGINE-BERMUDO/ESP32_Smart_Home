#include "app_web.h"
#include "app_ble.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "gateway_web";
static httpd_handle_t server = nullptr;

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

static esp_err_t offHandler(httpd_req_t *request)
{
    return handleCommand(request, false);
}

void setupWeb(void)
{
    if (server != nullptr) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = homeHandler},
        {.uri = "/app.js", .method = HTTP_GET, .handler = scriptHandler},
        {.uri = "/inter-latin.woff2", .method = HTTP_GET, .handler = fontHandler},
        {.uri = "/licenses.txt", .method = HTTP_GET, .handler = licenseHandler},
        {.uri = "/on", .method = HTTP_POST, .handler = onHandler},
        {.uri = "/off", .method = HTTP_POST, .handler = offHandler},
    };
    for (const auto &route : routes)
    {
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &route));
    }
    ESP_LOGI(TAG, "Web server started");
}
