cmake_minimum_required(VERSION 3.16.0)
# Add the shared helper while preserving PlatformIO's components.
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../common/ble")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(RelayNode)
