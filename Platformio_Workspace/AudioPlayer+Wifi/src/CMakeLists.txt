# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_driver_i2s esp_wifi esp_event esp_netif nvs_flash esp_http_server)

target_add_binary_data(${COMPONENT_LIB} "singing.wav" BINARY)


