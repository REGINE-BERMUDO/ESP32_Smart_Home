cmake_minimum_required(VERSION 3.16.0)
# All nodes use the same shared ESP-IDF components.
list(APPEND EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../Common/BLE"
    "${CMAKE_CURRENT_LIST_DIR}/../Common/LED"
)
# Build the gateway dependencies, not unrelated display drivers.
set(COMPONENTS src __pio_env)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(GatewayNode)

# ESP-IDF 6.1 exports an optional S3 MSPI include directory absent from this SDK.
# Remove it at the source so regenerated PlatformIO IntelliSense stays valid.
idf_component_get_property(hw_support_dir esp_hw_support COMPONENT_DIR)
idf_component_get_property(hw_support_lib esp_hw_support COMPONENT_LIB)
set(mspi_optional_include "${hw_support_dir}/mspi/mspi_timing_tuning/port/esp32s3/include")
if(NOT IS_DIRECTORY "${mspi_optional_include}")
    foreach(include_property INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(hw_support_includes ${hw_support_lib} ${include_property})
        if(hw_support_includes)
            list(REMOVE_ITEM hw_support_includes "${mspi_optional_include}")
            set_property(TARGET ${hw_support_lib} PROPERTY ${include_property} "${hw_support_includes}")
        endif()
    endforeach()
endif()
