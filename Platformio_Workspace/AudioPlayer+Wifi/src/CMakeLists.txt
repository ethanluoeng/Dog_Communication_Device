# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_driver_i2s esp_wifi esp_event esp_netif nvs_flash esp_http_server)

target_add_binary_data(${COMPONENT_LIB} "Down.wav" BINARY)
target_add_binary_data(${COMPONENT_LIB} "Find_Ethan.wav" BINARY)
target_add_binary_data(${COMPONENT_LIB} "Sit.wav" BINARY)
target_add_binary_data(${COMPONENT_LIB} "Who_Let_The_Dogs_Out.wav" BINARY)
target_add_binary_data(${COMPONENT_LIB} "GOOD_BOY.wav" BINARY)
target_add_binary_data(${COMPONENT_LIB} "Moo.wav" BINARY)

