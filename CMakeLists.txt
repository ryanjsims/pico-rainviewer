cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

include(lib/pico-sdk/pico_sdk_init.cmake)

project(pico-rainviewer)

pico_sdk_init()

add_subdirectory(lib/pico-web-client)
target_compile_definitions(pico_web_client PRIVATE "LOG_LEVEL=LOG_LEVEL_INFO")

add_library(PNGdec
    lib/PNGdec/src/adler32.c
    lib/PNGdec/src/crc32.c
    lib/PNGdec/src/infback.c
    lib/PNGdec/src/inffast.c
    lib/PNGdec/src/inflate.c
    lib/PNGdec/src/inftrees.c
    lib/PNGdec/src/PNGdec.cpp
    lib/PNGdec/src/zutil.c
)
target_include_directories(PNGdec PRIVATE lib/PNGdec/src)
target_compile_definitions(PNGdec PRIVATE "__MACH__=\"pico_w\"")


add_executable(pico_rainviewer
    src/main.cpp
    src/rgb_matrix.cpp
    src/weather_map.cpp
)

pico_generate_pio_header(pico_rainviewer ${CMAKE_CURRENT_LIST_DIR}/src/pio/hub75e.pio)
target_include_directories(pico_rainviewer PUBLIC include)
target_link_libraries(pico_rainviewer PRIVATE
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mbedtls
    pico_mbedtls
    pico_multicore
    pico_web_client
    hardware_rtc
    hardware_dma
    hardware_pio
    PNGdec
)
target_compile_options(pico_rainviewer PRIVATE "-Wno-psabi")
target_compile_definitions(pico_rainviewer PRIVATE "WIFI_SSID=\"$ENV{WIFI_SSID}\"" "WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"" "LOG_LEVEL=LOG_LEVEL_INFO")
target_link_options(pico_rainviewer PRIVATE "-Wl,--print-memory-usage")

pico_enable_stdio_usb(pico_rainviewer 1)
pico_enable_stdio_uart(pico_rainviewer 0)
pico_add_extra_outputs(pico_rainviewer)