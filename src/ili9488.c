/**
 * @file ili9488.c
 * @brief ILI9488 LCD driver implementation
 */

#include <stdio.h>
#include <string.h>
#include "../include/ili9488.h"
#include "ili9488_hal.h"

// Global variables to store LCD configuration
static struct {
    bool is_initialized;
    uint16_t width;
    uint16_t height;
    uint8_t rotation;
} ili9488 = {0};

// ILI9488 initialization sequence based on manufacturer example code
static void ili9488_init_sequence(void) {
    printf("Executing ILI9488 initialization sequence...\n");
    
    // Software reset
    ili9488_hal_write_cmd(ILI9488_SWRESET);
    ili9488_hal_delay_ms(200);

    // Exit sleep mode
    ili9488_hal_write_cmd(ILI9488_SLPOUT);
    ili9488_hal_delay_ms(200);
    
    // Set memory access control (MADCTL)
    ili9488_hal_write_cmd(ILI9488_MADCTL);
    ili9488_hal_write_data(0x48); // This command defines the read/write scan direction of the frame memory
    
    // Set pixel format - using 18-bit color according to GMT035-04-4SPI.ino example
    ili9488_hal_write_cmd(ILI9488_PIXFMT);
    ili9488_hal_write_data(0x66); // 18 bits/pixel (666 format)
    
    // VCOM control
    ili9488_hal_write_cmd(0xC5);
    ili9488_hal_write_data(0x00);
    ili9488_hal_write_data(0x36);
    ili9488_hal_write_data(0x80);
    
    // Power control
    ili9488_hal_write_cmd(0xC2);
    ili9488_hal_write_data(0xA7);
    
    // Positive gamma correction
    ili9488_hal_write_cmd(0xE0);
    ili9488_hal_write_data(0xF0);
    ili9488_hal_write_data(0x01);
    ili9488_hal_write_data(0x06);
    ili9488_hal_write_data(0x0F);
    ili9488_hal_write_data(0x12);
    ili9488_hal_write_data(0x1D);
    ili9488_hal_write_data(0x36);
    ili9488_hal_write_data(0x54);
    ili9488_hal_write_data(0x44);
    ili9488_hal_write_data(0x0C);
    ili9488_hal_write_data(0x18);
    ili9488_hal_write_data(0x16);
    ili9488_hal_write_data(0x13);
    ili9488_hal_write_data(0x15);
    
    // Negative gamma correction
    ili9488_hal_write_cmd(0xE1);
    ili9488_hal_write_data(0xF0);
    ili9488_hal_write_data(0x01);
    ili9488_hal_write_data(0x05);
    ili9488_hal_write_data(0x0A);
    ili9488_hal_write_data(0x0B);
    ili9488_hal_write_data(0x07);
    ili9488_hal_write_data(0x32);
    ili9488_hal_write_data(0x44);
    ili9488_hal_write_data(0x44);
    ili9488_hal_write_data(0x0C);
    ili9488_hal_write_data(0x18);
    ili9488_hal_write_data(0x17);
    ili9488_hal_write_data(0x13);
    ili9488_hal_write_data(0x16);
    
    // Invert display
    ili9488_hal_write_cmd(ILI9488_INVON);
    
    // Turn display on
    ili9488_hal_write_cmd(ILI9488_DISPON);
    
    ili9488_hal_delay_ms(50);
}

// Initialize ILI9488
bool ili9488_init(const ili9488_config_t *config) {
    printf("Initializing ILI9488 LCD...\n");
    
    if (ili9488.is_initialized) {
        printf("ILI9488 is already initialized\n");
        return true;
    }
    
    if (config == NULL) {
        printf("Error: Configuration is NULL\n");
        return false;
    }
    
    // Save LCD screen parameters
    ili9488.width = config->width;
    ili9488.height = config->height;
    ili9488.rotation = config->rotation;
    
    // Set hardware abstraction layer configuration
    ili9488_hw_config_t hw_config = {
        .spi_inst = config->spi_inst,
        .spi_speed_hz = config->spi_speed_hz,
        .pin_din = config->pin_din,
        .pin_sck = config->pin_sck,
        .pin_cs = config->pin_cs,
        .pin_dc = config->pin_dc,
        .pin_reset = config->pin_reset,
        .pin_bl = config->pin_bl
    };
    
    // Initialize hardware abstraction layer
    if (!ili9488_hal_init(&hw_config)) {
        printf("Error: Hardware initialization failed\n");
        return false;
    }
    
    // Perform hardware reset
    ili9488_hal_reset();
    
    // Run initialization sequence
    ili9488_init_sequence();
    
    // Set rotation direction
    ili9488_set_rotation(ili9488.rotation);
    
    ili9488.is_initialized = true;
    printf("ILI9488 initialization complete\n");
    
    return true;
}

// Set LCD backlight
void ili9488_set_backlight(bool on) {
    ili9488_hal_set_backlight(on);
}

// Set LCD backlight brightness
void ili9488_set_backlight_brightness(uint8_t brightness) {
    ili9488_hal_set_backlight_brightness(brightness);
}

// Set drawing window
void ili9488_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Set column address
    ili9488_hal_write_cmd(ILI9488_CASET);
    ili9488_hal_write_data(x0 >> 8);
    ili9488_hal_write_data(x0 & 0xFF);
    ili9488_hal_write_data(x1 >> 8);
    ili9488_hal_write_data(x1 & 0xFF);
    
    // Set row address
    ili9488_hal_write_cmd(ILI9488_PASET);
    ili9488_hal_write_data(y0 >> 8);
    ili9488_hal_write_data(y0 & 0xFF);
    ili9488_hal_write_data(y1 >> 8);
    ili9488_hal_write_data(y1 & 0xFF);
    
    // Prepare to write data
    ili9488_hal_write_cmd(ILI9488_RAMWR);
}

// Convert RGB565 color to RGB666 format (18-bit) - necessary for ILI9488
void rgb565_to_rgb666(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
    // Extract components from RGB565
    *r = (color >> 11) & 0x1F;  // 5-bit red
    *g = (color >> 5) & 0x3F;   // 6-bit green
    *b = color & 0x1F;          // 5-bit blue
    
    // Expand to RGB666 format (each color component uses 6 bits)
    *r = (*r << 1) | (*r >> 4); // Expand 5-bit red to 6-bit
    // Green is already 6-bit, no expansion needed
    *b = (*b << 1) | (*b >> 4); // Expand 5-bit blue to 6-bit
}

// Convert 24-bit RGB color to RGB666 format - new, according to manufacturer standard
void rgb24_to_rgb666(uint32_t color24, uint8_t *r, uint8_t *g, uint8_t *b) {
    // Extract RGB components - no shift, using direct values
    *r = (color24 >> 16) & 0xFF;
    *g = (color24 >> 8) & 0xFF;
    *b = color24 & 0xFF;
    
    // No need to convert, already in correct format for 18-bit RGB666
    // Manufacturer's code sends high 6 bits directly without any shift
}

// Draw a single pixel
void ili9488_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= ili9488.width || y >= ili9488.height) {
        return;  // Out of display bounds
    }
    
    ili9488_set_window(x, y, x, y);
    
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    // Send RGB666 format data (three bytes)
    uint8_t data[3] = {r, g, b};
    ili9488_hal_write_data_buffer(data, 3);
}

// Draw a single pixel (24-bit RGB format) - new
void ili9488_draw_pixel_rgb24(uint16_t x, uint16_t y, uint32_t color24) {
    if (x >= ili9488.width || y >= ili9488.height) {
        return;  // Out of display bounds
    }
    
    ili9488_set_window(x, y, x, y);
    
    // 直接使用颜色值的字节，不进行右移，符合厂商实现
    uint8_t r = (color24 >> 16) & 0xFF;
    uint8_t g = (color24 >> 8) & 0xFF;
    uint8_t b = color24 & 0xFF;
    
    // Send RGB666 format data (three bytes)
    uint8_t data[3] = {r, g, b};
    ili9488_hal_write_data_buffer(data, 3);
}

// Send data buffer to LCD
void ili9488_write_data_buffer(const uint8_t *data, size_t len) {
    ili9488_hal_write_data_buffer(data, len);
}

// Fill screen with a single color
void ili9488_fill_screen(uint16_t color) {
    // Use optimized area fill function
    ili9488_fill_area(0, 0, ili9488.width - 1, ili9488.height - 1, color);
}

// Fill screen with a single color (24-bit RGB format)
void ili9488_fill_screen_rgb24(uint32_t color24) {
    // Use optimized area fill function
    ili9488_fill_area_rgb24(0, 0, ili9488.width - 1, ili9488.height - 1, color24);
}

// Set display orientation
void ili9488_set_rotation(uint8_t rotation) {
    uint8_t madctl = 0;
    
    switch (rotation % 4) {
        case 0:  // 0 degree rotation
            madctl = ILI9488_MADCTL_MX | ILI9488_MADCTL_BGR;
            ili9488.width = 320;
            ili9488.height = 480;
            break;
        case 1:  // 90 degree rotation
            madctl = ILI9488_MADCTL_MV | ILI9488_MADCTL_BGR;
            ili9488.width = 480;
            ili9488.height = 320;
            break;
        case 2:  // 180 degree rotation
            madctl = ILI9488_MADCTL_MY | ILI9488_MADCTL_BGR;
            ili9488.width = 320;
            ili9488.height = 480;
            break;
        case 3:  // 270 degree rotation
            madctl = ILI9488_MADCTL_MX | ILI9488_MADCTL_MY | ILI9488_MADCTL_MV | ILI9488_MADCTL_BGR;
            ili9488.width = 480;
            ili9488.height = 320;
            break;
    }
    
    ili9488.rotation = rotation % 4;
    
    ili9488_hal_write_cmd(ILI9488_MADCTL);
    ili9488_hal_write_data(madctl);
}

/**
 * @brief Activate partial refresh mode
 * 
 * @param enable Whether to enable partial refresh mode
 */
void ili9488_partial_mode(bool enable) {
    if (enable) {
        // Enable partial refresh mode
        ili9488_hal_write_cmd(ILI9488_PTLON);
    } else {
        // Return to normal display mode
        ili9488_hal_write_cmd(ILI9488_NORON);
    }
    
    // Delay a short time to ensure the command takes effect
    ili9488_hal_delay_ms(10);
}

/**
 * @brief Set partial refresh area
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 */
void ili9488_set_partial_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Check if coordinates are valid
    if (x0 >= ili9488.width || y0 >= ili9488.height || 
        x1 >= ili9488.width || y1 >= ili9488.height) {
        return;
    }
    
    // Ensure starting coordinates are less than ending coordinates
    if (x0 > x1) {
        uint16_t temp = x0;
        x0 = x1;
        x1 = temp;
    }
    
    if (y0 > y1) {
        uint16_t temp = y0;
        y0 = y1;
        y1 = temp;
    }
    
    // Send partial area setting command
    ili9488_hal_write_cmd(ILI9488_PTLAR);
    ili9488_hal_write_data(y0 >> 8);
    ili9488_hal_write_data(y0 & 0xFF);
    ili9488_hal_write_data(y1 >> 8);
    ili9488_hal_write_data(y1 & 0xFF);
}

/**
 * @brief Batch write RGB565 pixel data (efficient area filling)
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param colors RGB565 color array
 * @param len Length of color array
 */
void ili9488_write_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t *colors, size_t len) {
    if (colors == NULL || len == 0) {
        return;
    }
    
    // Set window
    ili9488_set_window(x0, y0, x1, y1);
    
    // Calculate total pixels in the area
    uint32_t area_pixels = (uint32_t)(x1 - x0 + 1) * (y1 - y0 + 1);
    
    // Prepare buffer - for efficiency, send multiple pixels at once
    #define BUFFER_SIZE 32  // Buffer size (can be adjusted according to available memory)
    uint8_t buffer[BUFFER_SIZE * 3];  // 3 bytes per pixel (RGB666)
    
    size_t color_idx = 0;
    uint32_t pixels_remaining = area_pixels;
    
    while (pixels_remaining > 0) {
        // Determine number of pixels to process in current batch
        uint32_t pixels_to_process = pixels_remaining < BUFFER_SIZE ? pixels_remaining : BUFFER_SIZE;
        
        // Fill buffer
        for (uint32_t i = 0; i < pixels_to_process; i++) {
            // Get color, if color array is not long enough, use it cyclically
            uint16_t color = colors[color_idx];
            color_idx = (color_idx + 1) % len;  // Cycle through color array
            
            // Convert to RGB666 format
            uint8_t r, g, b;
            rgb565_to_rgb666(color, &r, &g, &b);
            
            // Fill buffer
            buffer[i * 3] = r;
            buffer[i * 3 + 1] = g;
            buffer[i * 3 + 2] = b;
        }
        
        // Send batch data
        ili9488_hal_write_data_buffer(buffer, pixels_to_process * 3);
        
        pixels_remaining -= pixels_to_process;
    }
    
    #undef BUFFER_SIZE
}

/**
 * @brief Batch write RGB888 pixel data (efficient area filling)
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param colors 24-bit RGB color array
 * @param len Length of color array
 */
void ili9488_write_pixels_rgb24(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint32_t *colors, size_t len) {
    if (colors == NULL || len == 0) {
        return;
    }
    
    // Set window
    ili9488_set_window(x0, y0, x1, y1);
    
    // Calculate total pixels in the area
    uint32_t area_pixels = (uint32_t)(x1 - x0 + 1) * (y1 - y0 + 1);
    
    // Prepare buffer - for efficiency, send multiple pixels at once
    #define BUFFER_SIZE 32  // Buffer size (can be adjusted according to available memory)
    uint8_t buffer[BUFFER_SIZE * 3];  // 3 bytes per pixel (RGB666)
    
    size_t color_idx = 0;
    uint32_t pixels_remaining = area_pixels;
    
    while (pixels_remaining > 0) {
        // Determine number of pixels to process in current batch
        uint32_t pixels_to_process = pixels_remaining < BUFFER_SIZE ? pixels_remaining : BUFFER_SIZE;
        
        // Fill buffer
        for (uint32_t i = 0; i < pixels_to_process; i++) {
            // Get color, if color array is not long enough, use it cyclically
            uint32_t color24 = colors[color_idx];
            color_idx = (color_idx + 1) % len;  // Cycle through color array
            
            // Convert to RGB666 format
            uint8_t r, g, b;
            rgb24_to_rgb666(color24, &r, &g, &b);
            
            // Fill buffer
            buffer[i * 3] = r;
            buffer[i * 3 + 1] = g;
            buffer[i * 3 + 2] = b;
        }
        
        // Send batch data
        ili9488_hal_write_data_buffer(buffer, pixels_to_process * 3);
        
        pixels_remaining -= pixels_to_process;
    }
    
    #undef BUFFER_SIZE
}

/**
 * @brief Fast fill rectangular area with a single color
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param color RGB565 color
 */
void ili9488_fill_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    // Ensure starting coordinates are less than ending coordinates
    if (x0 > x1) {
        uint16_t temp = x0;
        x0 = x1;
        x1 = temp;
    }
    
    if (y0 > y1) {
        uint16_t temp = y0;
        y0 = y1;
        y1 = temp;
    }
    
    // Set window
    ili9488_set_window(x0, y0, x1, y1);
    
    // Convert color format
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    // Calculate total pixels
    uint32_t total_pixels = (uint32_t)(x1 - x0 + 1) * (y1 - y0 + 1);
    
    // Prepare RGB666 data (3 bytes/pixel)
    uint8_t rgb_data[3] = {r, g, b};
    
    // If area is small, fill pixel by pixel directly
    if (total_pixels <= 256) {
        for (uint32_t i = 0; i < total_pixels; i++) {
            ili9488_hal_write_data_buffer(rgb_data, 3);
        }
        return;
    }
    
    // For large areas, use buffer batch filling for efficiency
    #define BUFFER_SIZE 256  // Buffer size (bytes = BUFFER_SIZE * 3)
    uint8_t buffer[BUFFER_SIZE * 3];
    
    // Pre-fill buffer
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        buffer[i * 3] = r;
        buffer[i * 3 + 1] = g;
        buffer[i * 3 + 2] = b;
    }
    
    // Fill in batches
    uint32_t remaining = total_pixels;
    
    while (remaining > 0) {
        uint32_t batch_size = remaining > BUFFER_SIZE ? BUFFER_SIZE : remaining;
        ili9488_hal_write_data_buffer(buffer, batch_size * 3);
        remaining -= batch_size;
    }
    
    #undef BUFFER_SIZE
}

/**
 * @brief Fast fill rectangular area with a single 24-bit RGB color
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param color24 24-bit RGB color
 */
void ili9488_fill_area_rgb24(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color24) {
    if (x0 >= ili9488.width || y0 >= ili9488.height || x1 >= ili9488.width || y1 >= ili9488.height) {
        return;  // Out of display bounds
    }
    
    if (x0 > x1) {
        uint16_t temp = x0;
        x0 = x1;
        x1 = temp;
    }
    
    if (y0 > y1) {
        uint16_t temp = y0;
        y0 = y1;
        y1 = temp;
    }
    
    uint32_t num_pixels = (x1 - x0 + 1) * (y1 - y0 + 1);
    printf("Filling area (%u,%u) to (%u,%u) with color 0x%06lX, %lu pixels\n", x0, y0, x1, y1, color24, num_pixels);
    
    ili9488_set_window(x0, y0, x1, y1);
    
    // 直接使用颜色值的字节，不进行右移，符合厂商实现
    uint8_t r = (color24 >> 16) & 0xFF;
    uint8_t g = (color24 >> 8) & 0xFF;
    uint8_t b = color24 & 0xFF;
    
    uint8_t color_data[3] = {r, g, b};
    
    // Optimize by sending pixels in batches to avoid SPI overhead
    const uint16_t batch_size = 128; // Number of pixels to send in one batch
    uint8_t batch_data[3 * batch_size];
    
    // Create a batch of color data
    for (uint16_t i = 0; i < batch_size; i++) {
        batch_data[i * 3] = color_data[0];
        batch_data[i * 3 + 1] = color_data[1];
        batch_data[i * 3 + 2] = color_data[2];
    }
    
    // Send batches
    uint32_t remaining = num_pixels;
    while (remaining > 0) {
        uint16_t to_send = (remaining > batch_size) ? batch_size : remaining;
        ili9488_hal_write_data_buffer(batch_data, to_send * 3);
        remaining -= to_send;
        
        // Print progress
        if (remaining % 10000 == 0 && remaining > 0) {
            printf("Remaining: %lu pixels\n", remaining);
        }
    }
}

/**
 * @brief Send data to LCD using DMA
 * 
 * @param data Data pointer
 * @param len Data length
 * @return bool Whether transfer started successfully
 */
bool ili9488_write_data_dma(const uint8_t *data, size_t len) {
    // Parameter check
    if (data == NULL || len == 0) {
        return false;
    }
    
    // Set data/command pin to data mode
    ili9488_hal_dc(1);  // DC high (data mode)
    
    // Use HAL layer's DMA transfer function
    return ili9488_hal_write_data_dma(data, len);
} 