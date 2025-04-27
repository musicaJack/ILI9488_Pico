/**
 * @file ili9488_demo.c
 * @brief ILI9488 LCD Driver Demo Program - Poetry Display
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

// Basic ASCII 5x7 font
// Each character occupies 5 columns, displayed as 5x7 dot matrix
static const unsigned char font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // Space
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x04, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // "\"
    0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x08, 0x14, 0x54, 0x54, 0x3C, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x7F, 0x10, 0x28, 0x44, 0x00, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x08, 0x08, 0x2A, 0x1C, 0x08, // ->
    0x08, 0x1C, 0x2A, 0x08, 0x08  // <-
};

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
static void demo_static_text(void);
static void ili9488_draw_centered_string(const char* str, uint16_t y, uint32_t color, uint32_t bgColor, uint8_t size);

// Helper function - Create RGB565 color (kept for compatibility)
static inline uint16_t ili9488_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
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
    
    // Extract RGB components
    r = (color >> 16) & 0xFF;  // Extract R component
    g = (color >> 8) & 0xFF;   // Extract G component
    b = color & 0xFF;          // Extract B component
    
    bg_r = (bg >> 16) & 0xFF;  // Extract background R component
    bg_g = (bg >> 8) & 0xFF;   // Extract background G component
    bg_b = bg & 0xFF;          // Extract background B component

    // Convert to RGB565 format
    uint16_t color565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    uint16_t bg565 = ((bg_r & 0xF8) << 8) | ((bg_g & 0xFC) << 3) | (bg_b >> 3);
    
    // Print debug info
    printf("Drawing string: %s\n", str);
    printf("Original color: 0x%06X (R:%d,G:%d,B:%d), RGB565: 0x%04X\n", color, r, g, b, color565);
    printf("Original background: 0x%06X (R:%d,G:%d,B:%d), RGB565: 0x%04X\n", bg, bg_r, bg_g, bg_b, bg565);
    
    // Call existing draw string function
    ili9488_draw_string(x, y, str, color565, bg565, size);
}

// Helper function - Draw centered string
static void ili9488_draw_centered_string(const char* str, uint16_t y, uint32_t color, uint32_t bgColor, uint8_t size) {
    // Calculate string width
    uint16_t width = strlen(str) * 6 * size;
    // Calculate x position to center the string
    uint16_t x = (SCREEN_WIDTH - width) / 2;
    
    // Draw the string
    ili9488_draw_string_rgb24(x, y, str, color, bgColor, size);
}

// Helper function - Direct RGB24 color drawing (ILI9488-specific)
static void draw_string_direct_rgb24(uint16_t x, uint16_t y, const char *str, uint32_t color24, uint32_t bg24, uint8_t size) {
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;
    
    printf("Direct RGB24 string drawing: %s\n", str);
    printf("Color: 0x%06X, Background: 0x%06X\n", color24, bg24);
    
    // Extract RGB components
    uint8_t fg_r = (color24 >> 16) & 0xFF;
    uint8_t fg_g = (color24 >> 8) & 0xFF;
    uint8_t fg_b = color24 & 0xFF;
    
    uint8_t bg_r = (bg24 >> 16) & 0xFF;
    uint8_t bg_g = (bg24 >> 8) & 0xFF;
    uint8_t bg_b = bg24 & 0xFF;
    
    // Process each character
    while (*str) {
        // Check for newline
        if (*str == '\n') {
            cursor_x = x;
            cursor_y += size * 8;
        }
        // Check for carriage return
        else if (*str == '\r') {
            cursor_x = x;
        }
        // Regular character
        else {
            // Look up font data based on ASCII code
            unsigned char c = *str;
            if (c < 32 || c > 127) c = '?'; // Use question mark for out-of-range chars
            c -= 32; // Font table starts at ASCII 32 (space)
            
            // Traverse each column of font data (5 columns)
            for (uint8_t i = 0; i < 5; i++) {
                uint8_t line = font5x7[c * 5 + i];
                
                // Draw each row (7 rows)
                for (uint8_t j = 0; j < 7; j++) {
                    uint32_t pixelColor;
                    if (line & 0x01) {
                        pixelColor = color24; // Foreground color
                    } else {
                        pixelColor = bg24;    // Background color
                    }
                    
                    // Draw pixel based on size
                    if (size == 1) {
                        ili9488_draw_pixel_rgb24(cursor_x + i, cursor_y + j, pixelColor);
                    } else {
                        ili9488_fill_area_rgb24(
                            cursor_x + i * size, 
                            cursor_y + j * size, 
                            cursor_x + i * size + size - 1, 
                            cursor_y + j * size + size - 1, 
                            pixelColor
                        );
                    }
                    
                    line >>= 1;
                }
            }
            
            cursor_x += size * 6; // Character width is 5, plus 1 pixel spacing
            
            // Automatic line wrap
            if (cursor_x > (SCREEN_WIDTH - size * 5)) {
                cursor_x = x;
                cursor_y += size * 8;
            }
        }
        
        str++; // Move to next character
    }
}

// Helper function - Draw centered string with direct RGB24 colors
static void draw_centered_string_rgb24(const char* str, uint16_t y, uint32_t color, uint32_t bgColor, uint8_t size) {
    // Calculate string width
    uint16_t width = strlen(str) * 6 * size;
    // Calculate x position to center the string
    uint16_t x = (SCREEN_WIDTH - width) / 2;
    
    // Draw the string
    draw_string_direct_rgb24(x, y, str, color, bgColor, size);
}

// Poetry text display demo
static void demo_static_text(void) {
    printf("Running poetry display demo with color cycling...\n");
    // Rotate screen 90 degrees
    ili9488_set_rotation(1);
    // Turn on backlight and immediately set to maximum brightness
    ili9488_set_backlight_brightness(255);
    
    // Define color combinations: background color and text color
    typedef struct {
        uint32_t bg_color;    // Background color
        uint32_t text_color;  // Text color
        const char* desc;     // Description
    } color_scheme_t;
    
    // Define 8 different color combinations
    color_scheme_t color_schemes[] = {
        {COLOR_BLACK, COLOR_WHITE, "Black BG + White text"},
        {COLOR_BLACK, COLOR_GREEN, "Black BG + Green text"},
        {COLOR_BLACK, COLOR_RED, "Black BG + Red text"},
        {COLOR_WHITE, COLOR_BLACK, "White BG + Black text"},
        {COLOR_WHITE, COLOR_GREEN, "White BG + Green text"},
        {COLOR_WHITE, COLOR_RED, "White BG + Red text"},
        {COLOR_BLUE, COLOR_BLACK, "Blue BG + Black text"},
        {COLOR_BLUE, COLOR_WHITE, "Blue BG + White text"}
    };
    
    // Poetry content
    const char* lines[] = {
        "Satellites whisper,",
        "pixels dance.",
        "Pico brings them",
        "both to life."
    };
    
    // Display different color combinations in a loop
    while (true) {
        // Iterate through all color combinations
        for (int scheme = 0; scheme < sizeof(color_schemes) / sizeof(color_schemes[0]); scheme++) {
            // Get current color combination
            uint32_t bg_color = color_schemes[scheme].bg_color;
            uint32_t text_color = color_schemes[scheme].text_color;
            
            // Print current color scheme info
            printf("Scheme %d: %s\n", scheme + 1, color_schemes[scheme].desc);
            
            // Fill background color
            ili9488_fill_screen_rgb24(bg_color);
            
            // Draw poetry
            for (int i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
                // Use the direct RGB24 drawing function
                draw_string_direct_rgb24(30, 80 + i * 40, lines[i], text_color, bg_color, 2);
            }
            
            // Display current color scheme info at bottom of screen
            char info[50];
            sprintf(info, "Scheme %d: %s", scheme + 1, color_schemes[scheme].desc);
            draw_string_direct_rgb24(30, 280, info, text_color, bg_color, 1);
            
            // Wait 10 seconds
            printf("Waiting 10 seconds...\n");
            for (int i = 0; i < 10; i++) {
                sleep_ms(1000);
                printf("Time remaining: %d seconds...\n", 9-i);
            }
        }
    }
}

// Main program entry point
int main() {
    // Initialize standard IO
    stdio_init_all();
    
    // Display startup information
    printf("\nILI9488 LCD Poetry Display Demo\n");
    printf("Version: 1.0.0\n");
    printf("Display: ILI9488 3.5-inch 320x480 SPI TFT\n\n");
    
    // Configure LCD
    ili9488_config_t config = {
        .spi_inst = spi0,        // Use SPI0
        .spi_speed_hz = 40000000, // 40 MHz (manufacturer recommended maximum rate)
        .pin_din = PIN_DIN,     // MOSI
        .pin_sck = PIN_SCK,     // SCK
        .pin_cs = PIN_CS,       // CS
        .pin_dc = PIN_DC,       // DC
        .pin_reset = PIN_RESET, // Reset
        .pin_bl = PIN_BL,       // Backlight
        .width = SCREEN_WIDTH,  // Screen width
        .height = SCREEN_HEIGHT, // Screen height
        .rotation = 0           // Default rotation
    };
    
    // Initialize display
    if (!ili9488_init(&config)) {
        printf("Error: Failed to initialize ILI9488 display\n");
        return -1;
    }
    
    // Set initial brightness
    ili9488_set_backlight_brightness(255); // 0-255
    
    // Run the poetry display demo
    printf("Starting poetry display demo...\n");
    demo_static_text();
    
    // Note: This code will never be reached because demo_static_text has an infinite loop
    printf("Demo completed successfully.\n");
    
    return 0;
} 