cmake_minimum_required(VERSION 3.13)

# Pull in Raspberry Pi Pico SDK (must be defined before project)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Project definition
project(ILI9488_Modern_CPP CXX C ASM)

# Compiler options for better code quality (reduced warnings for embedded)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -Wno-unused-parameter -Wno-pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -Wno-unused-parameter")

# Initialize the Pico SDK
pico_sdk_init()

# === Modern C++ ILI9488 Driver Library ===

# Source files for the modern C++ driver
set(MODERN_DRIVER_SOURCES
    src/ili9488_driver.cpp
    src/ili9488_ui.cpp
    src/hal/ili9488_hal.cpp
    src/fonts/ili9488_font.cpp
)

# Create the modern driver library
add_library(ili9488_modern_driver STATIC ${MODERN_DRIVER_SOURCES})

# Include directories
target_include_directories(ili9488_modern_driver PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Link Pico SDK libraries
target_link_libraries(ili9488_modern_driver PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_pwm
    hardware_dma
)

# === Legacy C API Compatibility Layer ===
# Note: Legacy wrapper removed as original C headers are not available

# === Example Programs ===

# Helper function to create example executables
function(create_example_target target_name source_file)
    add_executable(${target_name} ${source_file})
    
    target_link_libraries(${target_name}
        ili9488_modern_driver
        pico_stdlib
        hardware_spi
        hardware_gpio
        hardware_pwm
        hardware_dma
    )
    
    # Enable USB output
    pico_enable_stdio_usb(${target_name} 1)
    pico_enable_stdio_uart(${target_name} 0)
    
    # Generate UF2 output
    pico_add_extra_outputs(${target_name})
endfunction()

# Modern C++ demos
create_example_target(ili9488_modern_demo examples/ili9488_modern_demo.cpp)
create_example_target(ili9488_modern_optimization_demo examples/ili9488_modern_optimization_demo.cpp)
create_example_target(ili9488_modern_graphics_demo examples/ili9488_modern_graphics_demo.cpp)
create_example_target(ili9488_font_test examples/ili9488_font_test.cpp)
