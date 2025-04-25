/**
 * @file ili9488_optimization_demo.c
 * @brief ILI9488 optimization demo, showcasing GRAM acceleration functionality
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "ili9488.h"
#include "ili9488_gfx.h"

// Pin definitions - modified to match README
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_DC   20
#define PIN_RST  15  
#define PIN_BL   10  

// SPI instance
#define SPI_INST    spi0
#define SPI_BAUDRATE 40000000  // 40MHz

// Create a test pattern array
const uint16_t test_pattern[] = {
    0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000,  // Red, Green, Blue, White, Black
    0xFC00, 0x83E0, 0x801F, 0xF81F, 0xFFE0   // Orange, Cyan, Navy, Magenta, Yellow
};

const uint32_t test_pattern_rgb24[] = {
    0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF, 0x000000,  // Red, Green, Blue, White, Black
    0xFFA500, 0x00CED1, 0x000080, 0xFF00FF, 0xFFFF00   // Orange, Cyan, Navy, Magenta, Yellow
};

// Frame counter and time measurement for performance
uint32_t frame_count = 0;
absolute_time_t start_time;

/**
 * @brief Initialize LCD
 */
bool init_lcd(void) {
    printf("Starting LCD initialization...\n");
    
    // LCD configuration
    ili9488_config_t config = {
        .spi_inst = SPI_INST,
        .spi_speed_hz = SPI_BAUDRATE,
        .pin_din = PIN_MOSI,
        .pin_sck = PIN_SCK,
        .pin_cs = PIN_CS,
        .pin_dc = PIN_DC,
        .pin_reset = PIN_RST,
        .pin_bl = PIN_BL,
        .width = 320,
        .height = 480,
        .rotation = 0  // 0 degree rotation
    };
    
    // Initialize LCD
    bool result = ili9488_init(&config);
    if (result) {
        // Turn on backlight
        ili9488_set_backlight(true);
        printf("LCD initialization successful!\n");
    } else {
        printf("LCD initialization failed!\n");
    }
    
    return result;
}

/**
 * @brief Display performance test title
 */
void display_title(const char* title) {
    ili9488_fill_screen(0x0000);  // Clear screen
    
    // Display title
    printf("\n--- %s ---\n", title);
    
    // Reset frame counter
    frame_count = 0;
    start_time = get_absolute_time();
}

/**
 * @brief Display performance test results
 */
void display_performance(void) {
    absolute_time_t end_time = get_absolute_time();
    uint32_t elapsed_us = absolute_time_diff_us(start_time, end_time);
    float fps = (float)frame_count * 1000000 / elapsed_us;
    
    printf("Frames: %lu, Time: %.2f seconds, FPS: %.2f\n", 
           frame_count, elapsed_us / 1000000.0f, fps);
    
    // Wait a moment to observe results
    sleep_ms(1000);
}

/**
 * @brief Test 1: Standard drawing method vs optimized area filling
 */
void test_fill_optimization(void) {
    display_title("Standard filling vs Optimized filling");
    
    // Test standard filling method
    printf("Testing standard filling method...\n");
    start_time = get_absolute_time();
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 5; j++) {
            uint16_t color = test_pattern[j];
            
            // Use standard method to fill screen (one pixel per call)
            ili9488_set_window(0, 0, 319, 479);
            uint8_t r, g, b;
            rgb565_to_rgb666(color, &r, &g, &b);
            uint8_t data[3] = {r, g, b};
            
            for (int p = 0; p < 320 * 480; p++) {
                ili9488_hal_write_data_buffer(data, 3);
            }
        }
        frame_count++;
    }
    display_performance();
    
    // Test optimized filling method
    display_title("Optimized filling method");
    printf("Testing optimized filling method...\n");
    start_time = get_absolute_time();
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 5; j++) {
            // Use optimized area filling method
            ili9488_fill_area(0, 0, 319, 479, test_pattern[j]);
        }
        frame_count++;
    }
    display_performance();
}

/**
 * @brief Test 2: Partial update test
 */
void test_partial_update(void) {
    display_title("Partial update test");
    
    // Fill background first
    ili9488_fill_screen(0x0000);  // Black background
    
    // Enable partial update mode
    ili9488_partial_mode(true);
    
    printf("Partial update test in progress...\n");
    start_time = get_absolute_time();
    
    // Moving square
    int x = 0;
    int direction = 1;  // 1 for right, -1 for left
    
    // Set partial update area (full row)
    ili9488_set_partial_area(0, 100, 319, 200);
    
    while (frame_count < 100) {
        // Cover entire partial area with black
        ili9488_fill_area(0, 100, 319, 200, 0x0000);
        
        // Draw a color square at new position
        ili9488_fill_area(x, 100, x + 50, 200, test_pattern[frame_count % 5]);
        
        // Move square position
        x += direction * 5;
        if (x >= 270 || x <= 0) {
            direction *= -1;  // Reverse direction
        }
        
        frame_count++;
        sleep_ms(20);  // Limit frame rate to make animation visible
    }
    
    // Disable partial update mode
    ili9488_partial_mode(false);
    
    display_performance();
}

/**
 * @brief Test 3: Using DMA for bulk data transfer
 * Note: This requires DMA enabled in hardware layer
 */
void test_dma_transfer(void) {
    display_title("DMA transfer test");
    
    // Create a large test pattern array
    uint16_t pattern[320];  // Data for one row
    for (int i = 0; i < 320; i++) {
        pattern[i] = test_pattern[i % 5];
    }
    
    printf("DMA transfer test in progress...\n");
    start_time = get_absolute_time();
    
    while (frame_count < 30) {
        for (int row = 0; row < 480; row++) {
            // Use bulk pixel writing for each row
            ili9488_write_pixels(0, row, 319, row, pattern, 320);
        }
        frame_count++;
    }
    
    display_performance();
}

/**
 * @brief Test 4: Draw color gradient effect
 */
void test_gradient(void) {
    display_title("Gradient effect test");
    
    printf("Drawing color gradient...\n");
    start_time = get_absolute_time();
    
    while (frame_count < 10) {
        for (int y = 0; y < 480; y++) {
            // Create row gradient colors
            uint32_t row_colors[320];
            for (int x = 0; x < 320; x++) {
                // Create RGB color based on position
                uint8_t r = x * 255 / 320;
                uint8_t g = y * 255 / 480;
                uint8_t b = 128 + (x * y) % 128;
                row_colors[x] = (r << 16) | (g << 8) | b;
            }
            
            // Use RGB24 bulk writing
            ili9488_write_pixels_rgb24(0, y, 319, y, row_colors, 320);
        }
        frame_count++;
    }
    
    display_performance();
}

/**
 * @brief Program entry point
 */
int main(void) {
    stdio_init_all();
    sleep_ms(2000);  // Wait for serial connection
    printf("\nILI9488 Optimization Demo\n");
    
    // Initialize LCD
    if (!init_lcd()) {
        printf("Error: LCD initialization failed\n");
        return -1;
    }
    
    // Display welcome message
    ili9488_fill_screen(0x0000);
    ili9488_draw_string(10, 10, "ILI9488 Optimization Demo", 0xFFFF, 0x0000, 2);
    ili9488_draw_string(10, 50, "Demonstrating acceleration techniques", 0xFFFF, 0x0000, 1);
    sleep_ms(3000);
    
    // Run tests in sequence
    test_fill_optimization();
    test_partial_update();
    test_dma_transfer();
    test_gradient();
    
    // Final screen
    ili9488_fill_screen(0x0000);
    ili9488_draw_string(10, 10, "Tests completed", 0xFFFF, 0x0000, 2);
    ili9488_draw_string(10, 50, "Restarting in 3 seconds...", 0xFFFF, 0x0000, 1);
    sleep_ms(3000);
    
    // Restart program
    while (1) {
        test_fill_optimization();
        test_gradient();
        sleep_ms(2000);
    }
    
    return 0;
} 