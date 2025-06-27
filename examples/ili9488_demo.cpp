/**
 * @file ili9488_modern_demo.cpp
 * @brief Modern C++ Demo for ILI9488 LCD Driver
 */

#include <cstdio>
#include <memory>
#include <array>
#include <string_view>

#include "pico/stdlib.h"
#include "hardware/spi.h"

// Modern C++ ILI9488 driver includes
#include "ili9488_driver.hpp"
#include "pico_ili9488_gfx.hpp"
#include "ili9488_colors.hpp"
#include "ili9488_font.hpp"

// 统一引脚配置
#include "pin_config.hpp"

using namespace ili9488;
using namespace ili9488_colors;

int main() {
    stdio_init_all();
    printf("=== ILI9488 Modern C++ Demo ===\n");
    
    // Modern C++ initialization with RAII
    ILI9488Driver driver(ILI9488_GET_SPI_CONFIG());
    pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver> gfx(driver, 320, 480);
    
    if (!driver.initialize()) {
        printf("Failed to initialize display!\n");
        return -1;
    }
    
    // Set 180 degree rotation to fix upside-down display
    driver.setRotation(Rotation::Portrait_180);  // Fix upside-down display
    
    // Clear the entire screen to prevent display artifacts
    driver.fillScreen(rgb565::BLACK);
    sleep_ms(100);  // Allow display to stabilize
    
    driver.setBacklight(true);
    printf("Display initialized successfully with 180° rotation!\n");
    
    // Demo: Fill screen and draw shapes
    gfx.clearScreenFast(rgb565::WHITE);
    gfx.drawRect(10, 10, 100, 80, rgb565::RED);
    gfx.fillCircle(200, 50, 30, rgb565::BLUE);
    
    printf("Modern C++ demo completed!\n");
    
    while (true) {
        sleep_ms(1000);
    }
    
    return 0;
} 