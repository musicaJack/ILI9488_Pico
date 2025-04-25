/**
 * @file ili9488.h
 * @brief ILI9488 LCD driver header file
 */

#ifndef _ILI9488_H_
#define _ILI9488_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ili9488_hal.h"

/**
 * @brief LCD configuration structure
 */
typedef struct {
    spi_inst_t *spi_inst;    // SPI instance
    uint32_t spi_speed_hz;   // SPI speed (Hz)
    
    // Pin definitions
    uint8_t pin_din;         // MOSI pin
    uint8_t pin_sck;         // SCK pin
    uint8_t pin_cs;          // CS pin
    uint8_t pin_dc;          // Data/Command pin
    uint8_t pin_reset;       // Reset pin
    uint8_t pin_bl;          // Backlight pin
    
    // Screen parameters
    uint16_t width;          // Width
    uint16_t height;         // Height
    
    // Direction
    uint8_t rotation;        // Rotation direction
} ili9488_config_t;

/**
 * @brief Color definitions (RGB666 format, according to manufacturer standard)
 */
#define ILI9488_BLACK       0x000000  // Black
#define ILI9488_WHITE       0xFCFCFC  // White
#define ILI9488_RED         0xFC0000  // Red
#define ILI9488_GREEN       0x00FC00  // Green
#define ILI9488_BLUE        0x0000FC  // Blue
#define ILI9488_YELLOW      0xFCFC00  // Yellow
#define ILI9488_CYAN        0x00FCFC  // Cyan
#define ILI9488_MAGENTA     0xFC00FC  // Magenta

/* ILI9488 command definitions */
#define ILI9488_NOP        0x00     // No operation
#define ILI9488_SWRESET    0x01     // Software reset
#define ILI9488_RDDID      0x04     // Read display ID
#define ILI9488_SLPIN      0x10     // Enter sleep mode
#define ILI9488_SLPOUT     0x11     // Exit sleep mode
#define ILI9488_PTLON      0x12     // Partial mode on
#define ILI9488_NORON      0x13     // Normal display mode on
#define ILI9488_INVOFF     0x20     // Display inversion off
#define ILI9488_INVON      0x21     // Display inversion on
#define ILI9488_DISPOFF    0x28     // Display off
#define ILI9488_DISPON     0x29     // Display on
#define ILI9488_CASET      0x2A     // Column address set
#define ILI9488_PASET      0x2B     // Page (row) address set
#define ILI9488_RAMWR      0x2C     // Memory write
#define ILI9488_RAMRD      0x2E     // Memory read
#define ILI9488_PTLAR      0x30     // Partial area
#define ILI9488_VSCRDEF    0x33     // Vertical scrolling definition
#define ILI9488_MADCTL     0x36     // Memory access control
#define ILI9488_VSCRSADD   0x37     // Vertical scrolling start address
#define ILI9488_PIXFMT     0x3A     // Interface pixel format

// MADCTL bit definitions
#define ILI9488_MADCTL_MY  0x80     // Row address order selection (0=top to bottom, 1=bottom to top)
#define ILI9488_MADCTL_MX  0x40     // Column address order selection (0=left to right, 1=right to left)
#define ILI9488_MADCTL_MV  0x20     // Row/column exchange (0=normal, 1=exchanged)
#define ILI9488_MADCTL_ML  0x10     // Vertical refresh order (0=top to bottom, 1=bottom to top)
#define ILI9488_MADCTL_BGR 0x08     // BGR/RGB order (0=RGB, 1=BGR)
#define ILI9488_MADCTL_MH  0x04     // Horizontal refresh order (0=left to right, 1=right to left)

/**
 * @brief Initialize ILI9488 driver
 * 
 * @param config LCD configuration parameters
 * @return bool Whether initialization was successful
 */
bool ili9488_init(const ili9488_config_t *config);

/**
 * @brief Set LCD backlight
 * 
 * @param on Backlight state (true for on, false for off)
 */
void ili9488_set_backlight(bool on);

/**
 * @brief Set LCD backlight brightness
 * 
 * @param brightness Brightness level (0-255), 0 for off, 255 for maximum brightness
 */
void ili9488_set_backlight_brightness(uint8_t brightness);

/**
 * @brief Fill the entire screen with specified color
 * 
 * @param color Fill color (RGB565 format)
 */
void ili9488_fill_screen(uint16_t color);

/**
 * @brief Set drawing window
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 */
void ili9488_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief Draw a single pixel
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Pixel color
 */
void ili9488_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Draw a single pixel (24-bit RGB format)
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param color24 24-bit RGB color
 */
void ili9488_draw_pixel_rgb24(uint16_t x, uint16_t y, uint32_t color24);

/**
 * @brief Send data block to LCD
 * 
 * @param data Data pointer
 * @param len Data length
 */
void ili9488_write_data_buffer(const uint8_t *data, size_t len);

/**
 * @brief Set display orientation
 * 
 * @param rotation Rotation value (0-3)
 */
void ili9488_set_rotation(uint8_t rotation);

/**
 * @brief Fill screen with a single color (24-bit RGB format)
 * 
 * @param color24 24-bit RGB color
 */
void ili9488_fill_screen_rgb24(uint32_t color24);

/**
 * @brief Convert RGB565 color to RGB666 format (18-bit)
 * 
 * @param color RGB565 color
 * @param r Output red component (6-bit)
 * @param g Output green component (6-bit)
 * @param b Output blue component (6-bit)
 */
void rgb565_to_rgb666(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief Convert 24-bit RGB color to RGB666 format (18-bit)
 * 
 * @param color 24-bit RGB color
 * @param r Output red component (6-bit)
 * @param g Output green component (6-bit)
 * @param b Output blue component (6-bit)
 */
void rgb24_to_rgb666(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief Bulk write RGB565 pixel data (efficient area filling)
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param colors RGB565 color array
 * @param len Length of color array
 */
void ili9488_write_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t *colors, size_t len);

/**
 * @brief Bulk write RGB888 pixel data (efficient area filling)
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param colors 24-bit RGB color array
 * @param len Length of color array
 */
void ili9488_write_pixels_rgb24(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint32_t *colors, size_t len);

/**
 * @brief Fast fill rectangular area with a single color
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param color RGB565 color
 */
void ili9488_fill_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief Fast fill rectangular area with a single 24-bit RGB color
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param color24 24-bit RGB color
 */
void ili9488_fill_area_rgb24(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color24);

/**
 * @brief Activate partial refresh mode
 * 
 * @param enable Whether to enable partial refresh mode
 */
void ili9488_partial_mode(bool enable);

/**
 * @brief Set partial refresh area
 * 
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 */
void ili9488_set_partial_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief Send data to LCD using DMA
 * 
 * @param data Data pointer
 * @param len Data length
 * @return bool Whether transfer started successfully
 */
bool ili9488_write_data_dma(const uint8_t *data, size_t len);

#endif /* _ILI9488_H_ */ 