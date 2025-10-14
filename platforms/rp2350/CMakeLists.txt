cmake_minimum_required(VERSION 3.13)

# Pico SDK path (set this to your SDK location)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(tinyol_kmeans C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Core clustering library
add_library(streaming_kmeans 
    ../../core/clustering/streaming_kmeans.c
)

target_include_directories(streaming_kmeans PUBLIC
    ../../core/clustering
)

# Platform wrapper library
add_library(kmeans_platform
    kmeans_platform.c
)

target_link_libraries(kmeans_platform
    streaming_kmeans
    pico_stdlib
    pico_cyw43_arch_none
)

target_include_directories(kmeans_platform PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# Main test executable
add_executable(kmeans_test
    main.c
)

target_link_libraries(kmeans_test
    kmeans_platform
    pico_stdlib
)

# Enable USB output, disable UART
pico_enable_stdio_usb(kmeans_test 1)
pico_enable_stdio_uart(kmeans_test 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(kmeans_test)