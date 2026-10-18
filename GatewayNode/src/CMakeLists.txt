# Keep the page assets in flash so the dashboard works without external services.
idf_component_register(
    SRCS "app_ble.cpp" "app_web.cpp" "app_wifi.cpp" "main.cpp"
    REQUIRES BLE LED esp_http_server esp_wifi esp_event esp_netif nvs_flash esp_timer
    EMBED_FILES "../web/index.html" "../web/app.js"
                "../web/inter-latin.woff2" "../web/licenses.txt"
)
