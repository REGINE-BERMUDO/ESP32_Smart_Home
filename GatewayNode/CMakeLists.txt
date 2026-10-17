cmake_minimum_required(VERSION 3.16.0)
# All nodes use the same shared ESP-IDF components.
list(APPEND EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../Common/BLE"
    "${CMAKE_CURRENT_LIST_DIR}/../Common/LED"
)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(GatewayNode)
