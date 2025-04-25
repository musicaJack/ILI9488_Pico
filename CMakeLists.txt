# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.12)

# Specify Pico SDK path (if needed)
# set(PICO_SDK_PATH "your Pico SDK path")

# Set Pico board type - use string format
set(PICO_BOARD "pico")

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Completely disable picotool and other tool downloads to avoid network issues
set(PICO_SKIP_TOOLS_PACKAGE_SEARCH 1)
set(SKIP_TOOL_RISCV_OPENOCD 1)
set(SKIP_TOOL_PICOTOOL 1)
set(PICO_SDK_FETCH_FROM_GIT 0)
set(PICOTOOL_FETCH_FROM_GIT 0)
set(PICO_SKIP_TOOL_CHECKS 1)
set(PICO_NO_HARDWARE 0)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${PICO_SDK_PATH}/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Include Pico SDK import script
include(pico_sdk_import.cmake)

# Project name 
project(ILI9488_Pico C CXX ASM)

# Define source files
set(LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
set(EXAMPLES_DIR ${CMAKE_CURRENT_LIST_DIR}/examples)

set(LIB_SOURCES
    ${LIB_DIR}/ili9488.c
    ${LIB_DIR}/ili9488_hal.c
    ${LIB_DIR}/ili9488_gfx.c
    ${LIB_DIR}/ili9488_font.c
)

# Include directories
include_directories(${INCLUDE_DIR})

# Initialize project (must be done before pico_sdk_init)
project(ILI9488_Pico C CXX ASM)

# Initialize Pico SDK
pico_sdk_init()

# Add library files
add_library(ili9488_driver STATIC ${LIB_SOURCES})

target_include_directories(ili9488_driver PUBLIC
    ${INCLUDE_DIR}
)

# Link Pico SDK related libraries
target_link_libraries(ili9488_driver PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_pwm
    hardware_dma  # Add DMA support
)

# Add example programs
# Compile basic example program
add_executable(ili9488_demo ${EXAMPLES_DIR}/ili9488_demo.c)
target_link_libraries(ili9488_demo
    ili9488_driver
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_pwm
    hardware_dma  # Add DMA support
)

# Set output files
pico_add_extra_outputs(ili9488_demo)

# Compile optimization demo program
add_executable(ili9488_optimization_demo ${EXAMPLES_DIR}/ili9488_optimization_demo.c)
target_link_libraries(ili9488_optimization_demo
    ili9488_driver
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_pwm
    hardware_dma
)
pico_add_extra_outputs(ili9488_optimization_demo)

# Compile physical dice simulation demo program
add_executable(ili9488_dice_physics_demo ${EXAMPLES_DIR}/ili9488_dice_physics_demo.c)
target_link_libraries(ili9488_dice_physics_demo
    ili9488_driver 
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_pwm
    hardware_dma
)
pico_add_extra_outputs(ili9488_dice_physics_demo)

# Enable USB output, disable UART output
pico_enable_stdio_usb(ili9488_demo 1)
pico_enable_stdio_uart(ili9488_demo 0)

pico_enable_stdio_usb(ili9488_optimization_demo 1)
pico_enable_stdio_uart(ili9488_optimization_demo 0)

pico_enable_stdio_usb(ili9488_dice_physics_demo 1)
pico_enable_stdio_uart(ili9488_dice_physics_demo 0)
