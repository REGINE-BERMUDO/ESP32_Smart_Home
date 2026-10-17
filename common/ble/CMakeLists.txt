# Build the shared Bluetooth helper.
idf_component_register(
    SRCS "ble_mesh.cpp"
    INCLUDE_DIRS "."
    REQUIRES bt freertos esp_system
)