/**
 * @file ili9488_demo.c
 * @brief ILI9488 LCD Driver Demo Program
 */

#include <stdio.h>
#include <string.h>  // Add string.h header file, provides strlen function declaration
#include "pico/stdlib.h"
#include "ili9488.h"
#include "ili9488_gfx.h"
#include <math.h> // Add header file for sin function

// Pin definitions - keep consistent with the original lcd_demo.c
#define PIN_DIN   19  // SPI MOSI
#define PIN_SCK   18  // SPI SCK
#define PIN_CS    17  // SPI CS - manually controlled
#define PIN_DC    20  // Data/Command
#define PIN_RESET 15  // Reset
#define PIN_BL    10  // Backlight

// Screen parameters - ILI9488
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 480

// Color definitions (24-bit RGB format) - using constants defined in ILI9488
#define COLOR_RED     ILI9488_RED     // Red
#define COLOR_GREEN   ILI9488_GREEN   // Green
#define COLOR_BLUE    ILI9488_BLUE    // Blue
#define COLOR_WHITE   ILI9488_WHITE   // White
#define COLOR_BLACK   ILI9488_BLACK   // Black
#define COLOR_YELLOW  0xFCFC00        // Yellow (Red+Green)
#define COLOR_CYAN    0x00FCFC        // Cyan (Green+Blue)
#define COLOR_MAGENTA 0xFC00FC        // Magenta (Red+Blue)
#define COLOR_ORANGE  0xFC7800        // Orange
#define COLOR_PURPLE  0x7800FC        // Purple
#define COLOR_LIME    0x78FC00        // Lime

// Function forward declarations
static void demo_static_graphics(void);
static void demo_color_animation(void);
static void demo_color_test(void);
static void demo_gradient_transition(void);
static void demo_brightness_checkboard(void);
static void demo_poetry(void);
static void ili9488_draw_centered_string(const char* str, uint16_t y, uint32_t color, uint32_t bgColor, uint8_t size);

// Helper function - Create RGB565 color (kept for compatibility)
static inline uint16_t ili9488_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

// Helper function - Create RGB666 color (designed according to manufacturer code)
// Note: This function returns a 24-bit color value, high 6 bits R, middle 6 bits G, low 6 bits B
static inline uint32_t ili9488_color_666(uint8_t r, uint8_t g, uint8_t b) {
    // Limit to 6-bit range (0-63)
    r &= 0x3F;
    g &= 0x3F;
    b &= 0x3F;
    
    // Convert to 24-bit color value (shift each channel left 2 bits, consistent with manufacturer code)
    return ((uint32_t)(r << 2) << 16) | ((uint32_t)(g << 2) << 8) | (b << 2);
}

// Helper function - Extract RGB components from 24-bit RGB color and convert to RGB666 format
static void RGB24_to_RGB666(uint32_t color24, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (color24 >> 16) & 0xFF;  // Extract R component (high 8 bits)
    *g = (color24 >> 8) & 0xFF;   // Extract G component (middle 8 bits)
    *b = color24 & 0xFF;          // Extract B component (low 8 bits)
    
    // Convert 8-bit components to 6-bit components
    *r = *r >> 2;  // 8-bit to 6-bit
    *g = *g >> 2;  // 8-bit to 6-bit
    *b = *b >> 2;  // 8-bit to 6-bit
}

// Helper function - Draw string using 24-bit RGB color
static void ili9488_draw_string_rgb24(uint16_t x, uint16_t y, const char *str, uint32_t color, uint32_t bg, uint8_t size) {
    uint8_t r, g, b, bg_r, bg_g, bg_b;
    
    // Extract RGB components from 24-bit RGB color
    RGB24_to_RGB666(color, &r, &g, &b);
    RGB24_to_RGB666(bg, &bg_r, &bg_g, &bg_b);
    
    // Convert to RGB565 format (compatible with existing draw string function)
    uint16_t color565 = ili9488_color(r >> 1, g, b >> 1); // Reduce 6-bit to 5-bit
    uint16_t bg565 = ili9488_color(bg_r >> 1, bg_g, bg_b >> 1);
    
    // Call existing draw string function
    ili9488_draw_string(x, y, str, color565, bg565, size);
}

// Static text demo
static void demo_static_text(void) {
    printf("Running static graphics demo...\n");
    // Rotate screen 90 degrees
    ili9488_set_rotation(1);
    // Turn on backlight and immediately set to maximum brightness
    ili9488_set_backlight_brightness(255);
    // Fill entire screen with black
    ili9488_fill_screen_rgb24(COLOR_BLUE);
    
    // Draw text - white text, black background
    printf("Drawing text...\n");
    ili9488_draw_string_rgb24(30, 80, "Satellites whisper,", ILI9488_WHITE, ILI9488_BLUE, 2);
    ili9488_draw_string_rgb24(30, 120, "pixels dance.", ILI9488_WHITE, ILI9488_BLUE, 2);
    ili9488_draw_string_rgb24(30, 160, "Pico brings them", ILI9488_WHITE, ILI9488_BLUE, 2);
    ili9488_draw_string_rgb24(30, 200, "both to life.", ILI9488_WHITE, ILI9488_BLUE, 2);
    sleep_ms(5000);
}


// Color animation demo
static void demo_color_animation(void) {
    printf("Running color animation demo...\n");
    
    // Set maximum brightness
    ili9488_set_backlight_brightness(255);
    
    uint32_t t = 0;
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t frames = 0;
    
    // Run animation for 10 seconds
    while (to_ms_since_boot(get_absolute_time()) - start_time < 10000) {
        uint8_t r, g, b;
        
        // Set drawing window to entire screen
        ili9488_set_window(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
        
        // Draw dynamic color pattern - use sine function to produce more vivid colors
        for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
            for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
                // Use time-related sine functions to generate more vivid colors
                float phase_r = (float)(x + t * 2) / 128.0f;
                float phase_g = (float)(y - t) / 128.0f;
                float phase_b = (float)(x + y + t) / 256.0f;
                
                // Generate RGB components in range 0-63 (6-bit)
                r = (uint8_t)(31.5f + 31.5f * sin(phase_r));
                g = (uint8_t)(31.5f + 31.5f * sin(phase_g));
                b = (uint8_t)(31.5f + 31.5f * sin(phase_b));
                
                // Send data directly using RGB666 format
                uint8_t data[3] = {r, g, b};
                ili9488_write_data_buffer(data, 3);
            }
        }
        
        t++;
        frames++;
        
        if (frames % 10 == 0) {
            printf("Completed %lu frames\n", frames);
        }
    }
    
    float fps = (float)frames / 10.0f;
    printf("Animation completed: %lu frames, average %.1f FPS\n", frames, fps);
}

// Color test
static void demo_color_test(void) {
    printf("Running color test...\n");
    
    // Set brightness to maximum
    ili9488_set_backlight_brightness(255);
    
    // Use manufacturer-defined colors (24-bit RGB format)
    printf("Starting color test...\n");
    
    // Red
    ili9488_fill_screen_rgb24(COLOR_RED);
    printf("Displaying red...\n");
    sleep_ms(1000);
    
    // Green
    ili9488_fill_screen_rgb24(COLOR_GREEN);
    printf("Displaying green...\n");
    sleep_ms(1000);
    
    // Blue
    ili9488_fill_screen_rgb24(COLOR_BLUE);
    printf("Displaying blue...\n");
    sleep_ms(1000);
    
    // Yellow
    ili9488_fill_screen_rgb24(COLOR_YELLOW);
    printf("Displaying yellow...\n");
    sleep_ms(1000);
    
    // Cyan
    ili9488_fill_screen_rgb24(COLOR_CYAN);
    printf("Displaying cyan...\n");
    sleep_ms(1000);
    
    // Magenta
    ili9488_fill_screen_rgb24(COLOR_MAGENTA);
    printf("Displaying magenta...\n");
    sleep_ms(1000);
    
    // Orange
    ili9488_fill_screen_rgb24(COLOR_ORANGE);
    printf("Displaying orange...\n");
    sleep_ms(1000);
    
    // Purple
    ili9488_fill_screen_rgb24(COLOR_PURPLE);
    printf("Displaying purple...\n");
    sleep_ms(1000);
    
    // Black
    ili9488_fill_screen_rgb24(COLOR_BLACK);
    printf("Displaying black...\n");
    sleep_ms(1000);
    
    // White
    ili9488_fill_screen_rgb24(COLOR_WHITE);
    printf("Displaying white...\n");
    sleep_ms(1000);
}

// Gradient transition display (new addition)
static void demo_gradient_transition(void) {
    printf("Running gradient transition display...\n");
    
    // Set maximum brightness
    ili9488_set_backlight_brightness(255);
    
    // Calculate total frame count (assuming 256 color transitions in 10 seconds, about 25 frames/second)
    const uint16_t total_frames = 250; // A little less than 256, leaving some room
    const uint32_t duration_ms = 10000; // Total duration 10 seconds
    const uint32_t frame_time_ms = duration_ms / total_frames;
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t frames = 0;
    
    // Set drawing window to entire screen
    ili9488_set_window(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    
    while (frames < total_frames) {
        // Calculate current gradient value (0-255)
        uint8_t gradient = (frames * 256) / total_frames;
        
        // Create gradient color - use manufacturer color definition method, more vivid
        // Transition from blue to red, through green and other colors
        uint8_t r, g, b;
        
        // Use sine function to produce smoother color transition
        float phase = (float)frames / total_frames * 3.0f; // Three-cycle phase
        
        // Generate RGB components in range 0-63 (6-bit)
        r = (uint8_t)(31.5f + 31.5f * sin(phase * 3.14159f));
        g = (uint8_t)(31.5f + 31.5f * sin((phase + 0.33f) * 2.0f * 3.14159f));
        b = (uint8_t)(31.5f + 31.5f * sin((phase + 0.66f) * 3.14159f));
        
        // Send data directly using RGB666 format
        // Fill entire screen
        for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
            for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
                uint8_t data[3] = {r, g, b};
                ili9488_write_data_buffer(data, 3);
            }
        }
        
        frames++;
        
        // Print progress
        if (frames % 25 == 0) {
            printf("Gradient transition: %.1f%%\n", (float)frames * 100.0f / total_frames);
        }
        
        // Control frame rate
        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - start_time;
        uint32_t target_time = frames * frame_time_ms;
        if (elapsed < target_time) {
            sleep_ms(target_time - elapsed);
        }
    }
    
    printf("Gradient transition completed\n");
}

// Brightness change black and white squares and RGB color demonstration (new addition)
static void demo_brightness_checkboard(void) {
    printf("Running brightness change black and white squares and RGB color demonstration...\n");
    
    // Since this demonstration is specifically demonstrating brightness change, no fixed brightness is set
    // But ensure that the second stage RGB color part uses maximum brightness
    
    // Total duration 20 seconds, divided into two stages: black and white change and RGB change
    const uint32_t total_duration_ms = 20000;
    const uint32_t bw_duration_ms = 10000; // Black and white stage 10 seconds
    const uint32_t rgb_duration_ms = 10000; // RGB stage 10 seconds
    
    const uint8_t grid_size = 20; // Square size
    const uint16_t cols = SCREEN_WIDTH / grid_size;
    const uint16_t rows = SCREEN_HEIGHT / grid_size;
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t current_time;
    
    // Stage 1: Black and white brightness change (brightness from 0 to 255)
    printf("Black and white square brightness change...\n");
    uint8_t brightness_steps = 100; // Brightness level
    for (uint8_t step = 0; step < brightness_steps; step++) {
        // Calculate current brightness
        uint8_t brightness = (step * 255) / (brightness_steps - 1);
        
        // Adjust black and white square color based on brightness, limit to RGB666 format range (0-63)
        uint8_t dark_value = 0;
        uint8_t light_value = brightness >> 2; // Convert 8-bit brightness to 6-bit (0-63)
        if (light_value > 63) light_value = 63; // Ensure within range
        
        // Draw black and white squares
        for (uint16_t row = 0; row < rows; row++) {
            for (uint16_t col = 0; col < cols; col++) {
                uint16_t x = col * grid_size;
                uint16_t y = row * grid_size;
                uint8_t value = ((row + col) % 2 == 0) ? light_value : dark_value;
                
                // Use RGB666 format to set window
                ili9488_set_window(x, y, x + grid_size - 1, y + grid_size - 1);
                
                // Fill square
                uint8_t data[3] = {value, value, value};
                for (uint16_t i = 0; i < grid_size * grid_size; i++) {
                    ili9488_write_data_buffer(data, 3);
                }
            }
        }
        
        // Print progress
        if (step % 10 == 0) {
            printf("Black and white brightness: %d/255 (%.1f%%)\n", brightness, (float)step * 100.0f / brightness_steps);
        }
        
        // Control time
        current_time = to_ms_since_boot(get_absolute_time()) - start_time;
        uint32_t target_time = (step * bw_duration_ms) / brightness_steps;
        if (current_time < target_time) {
            sleep_ms(target_time - current_time);
        }
        
        // Check if timeout
        if (current_time >= bw_duration_ms) {
            break;
        }
    }
    
    // Stage 2: RGB color change
    printf("RGB color square change...\n");
    const uint8_t color_steps = 100; // Color change times
    
    for (uint8_t step = 0; step < color_steps; step++) {
        // Brightness remains maximum
        ili9488_set_backlight_brightness(255);
        
        // Calculate RGB color intensity based on step (produce color loop)
        float phase = (float)step / color_steps * 3.0f;
        
        // Generate RGB666 format color components (0-63 range)
        uint8_t r_intensity = (uint8_t)(31.5f + 31.5f * sin(phase * 3.14159f));
        uint8_t g_intensity = (uint8_t)(31.5f + 31.5f * sin((phase + 0.33f) * 3.14159f));
        uint8_t b_intensity = (uint8_t)(31.5f + 31.5f * sin((phase + 0.66f) * 3.14159f));
        
        // Draw RGB squares
        for (uint16_t row = 0; row < rows; row++) {
            for (uint16_t col = 0; col < cols; col++) {
                uint16_t x = col * grid_size;
                uint16_t y = row * grid_size;
                
                // Set window
                ili9488_set_window(x, y, x + grid_size - 1, y + grid_size - 1);
                
                uint16_t pattern = (row + col) % 3;
                uint8_t r = (pattern == 0) ? r_intensity : 0;
                uint8_t g = (pattern == 1) ? g_intensity : 0;
                uint8_t b = (pattern == 2) ? b_intensity : 0;
                
                // Fill square
                uint8_t data[3] = {r, g, b};
                for (uint16_t i = 0; i < grid_size * grid_size; i++) {
                    ili9488_write_data_buffer(data, 3);
                }
            }
        }
        
        // Print progress
        if (step % 10 == 0) {
            printf("RGB color change: %.1f%%\n", (float)step * 100.0f / color_steps);
        }
        
        // Control time
        current_time = to_ms_since_boot(get_absolute_time()) - start_time - bw_duration_ms;
        uint32_t target_time = (step * rgb_duration_ms) / color_steps;
        if (current_time < target_time) {
            sleep_ms(target_time - current_time);
        }
        
        // Check if timeout
        if (to_ms_since_boot(get_absolute_time()) - start_time >= total_duration_ms) {
            break;
        }
    }
    
    printf("Brightness change black and white squares and RGB color demonstration completed\n");
}

int main() {
    // Initialize standard library
    stdio_init_all();
    sleep_ms(3000);  // Wait for serial initialization
    printf("\n\n\nStarting ILI9488 LCD Demo Program...\n");
    
    // Configure LCD
    ili9488_config_t config = {
        .spi_inst = spi0,
        .spi_speed_hz = 40 * 1000 * 1000,  // 40MHz
        
        .pin_din = PIN_DIN,
        .pin_sck = PIN_SCK,
        .pin_cs = PIN_CS,
        .pin_dc = PIN_DC,
        .pin_reset = PIN_RESET,
        .pin_bl = PIN_BL,
        
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .rotation = 0,  // 0 degree rotation
    };
    
    // initialize the LCD
    if (!ili9488_init(&config)) {
        printf("Error: LCD initialization failed\n");
        return -1;
    }

    // // 运行颜色测试
    // demo_color_test();
    
    // 运行静态文字演示
    demo_static_text();
    
    // 运行动态颜色动画演示
    //demo_color_animation();
    
    // 运行新添加的演示功能
    // demo_gradient_transition();       // New: Gradient transition
    // demo_brightness_checkboard();     // New: Brightness change squares



    printf("Demo completed.\n");
    
    // Keep displaying last frame
    while (true) {
        sleep_ms(1000);
    }
    
    return 0;
} 