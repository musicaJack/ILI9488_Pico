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

# Include directories for ILI9488
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

# === Joystick Driver Library ===

# Source files for the joystick driver
set(JOYSTICK_SOURCES
    src/joystick/joystick.cpp
)

# Create the joystick driver library
add_library(joystick_driver STATIC ${JOYSTICK_SOURCES})

# Include directories for joystick
target_include_directories(joystick_driver PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/joystick
)

# Link Pico SDK libraries for joystick
target_link_libraries(joystick_driver PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_gpio
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

# Helper function to create joystick-based example executables
function(create_joystick_example_target target_name source_file)
    add_executable(${target_name} ${source_file})
    
    target_link_libraries(${target_name}
        ili9488_modern_driver
        joystick_driver
        pico_stdlib
        hardware_spi
        hardware_gpio
        hardware_pwm
        hardware_dma
        hardware_i2c
    )
    
    # Enable USB output
    pico_enable_stdio_usb(${target_name} 1)
    pico_enable_stdio_uart(${target_name} 0)
    
    # Generate UF2 output
    pico_add_extra_outputs(${target_name})
endfunction()

# Modern C++ demos
create_example_target(ili9488_demo examples/ili9488_demo.cpp)
create_example_target(ili9488_optimization_demo examples/ili9488_optimization_demo.cpp)
create_example_target(ili9488_graphics_demo examples/ili9488_graphics_demo.cpp)
create_example_target(ili9488_font_test examples/ili9488_font_test.cpp)

# Joystick-based games and demos
create_joystick_example_target(CollisionX examples/CollisionX.cpp)
create_joystick_example_target(SnakeGame examples/SnakeGame.cpp)

# === MicroSD Library ===

# Add PIO SPI library
add_subdirectory(lib/pico_fatfs/pio/spi)

# Source files for MicroSD functionality
set(MICROSD_SOURCES
    src/microsd/rw_sd.cpp
    src/microsd/storage_device.cpp
)

# Create the MicroSD library
add_library(microsd_driver STATIC ${MICROSD_SOURCES})

# Include directories for MicroSD
target_include_directories(microsd_driver PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/microsd
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_fatfs
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_fatfs/fatfs
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_fatfs/pio/spi
    ${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include
)

# Link Pico SDK libraries for MicroSD
target_link_libraries(microsd_driver PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    pio_spi
)

# === Font System Library ===

# Source files for font system
set(FONT_SYSTEM_SOURCES
    src/fonts/hybrid_font_system.cpp
    src/fonts/flash_font_cache.cpp
)

# Create the font system library
add_library(font_system STATIC ${FONT_SYSTEM_SOURCES})

# Include directories for font system
target_include_directories(font_system PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/fonts
)

# Link Pico SDK libraries for font system
target_link_libraries(font_system PUBLIC
    pico_stdlib
)

# Pull in pico_fatfs library
add_subdirectory(lib/pico_fatfs)

# Helper function to create text reader example executables
function(create_text_reader_example_target target_name source_file)
    add_executable(${target_name} ${source_file})
    
    target_link_libraries(${target_name}
        ili9488_modern_driver
        joystick_driver
        microsd_driver
        font_system
        pico_stdlib
        hardware_spi
        hardware_gpio
        hardware_pwm
        hardware_dma
        hardware_i2c
        pico_fatfs
    )
    
    # Enable USB output
    pico_enable_stdio_usb(${target_name} 1)
    pico_enable_stdio_uart(${target_name} 0)
    
    # Generate UF2 output
    pico_add_extra_outputs(${target_name})
endfunction()

# Text reader applications
create_text_reader_example_target(ILI9488_TextReader examples/ILI9488_TextReader.cpp)
