cmake_minimum_required(VERSION 3.16)

# ESP-IDF project configuration
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(predictive_maintenance)

# Component dependencies are auto-resolved by ESP-IDF build system
# Explicit includes if needed:
# idf_component_register(
#     SRCS "main/main.c" "main/sensors.c" "main/filter.c"
#          "main/anomaly.c" "main/alert.c" "main/comm.c"
#     INCLUDE_DIRS "main"
#     REQUIRES driver esp_adc ledc nvs_flash esp_timer
# )
