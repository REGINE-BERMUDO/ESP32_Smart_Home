# Shared BLE helper for C and C++ nodes.
idf_component_register(
    SRCS "ble_mesh.c"
    INCLUDE_DIRS "."
    REQUIRES bt freertos esp_system nvs_flash
)
