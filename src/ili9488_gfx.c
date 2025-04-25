/**
 * @file ili9488_gfx.c
 * @brief ILI9488 LCD graphics function library implementation
 */

#include <stdlib.h>
#include "ili9488_gfx.h"
#include "ili9488.h"

// Note: rgb565_to_rgb666 and rgb24_to_rgb666 functions are now implemented in ili9488.c
// and declared in ili9488.h

// Declaration of circle filling helper function
static void ili9488_fill_circle_helper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t corners, uint16_t delta, uint16_t color);

// Draw horizontal line
void ili9488_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    if (w <= 0) return;
    
    // Fast horizontal line drawing, set window then batch transfer the same color data
    ili9488_set_window(x, y, x + w - 1, y);
    
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    for (uint16_t i = 0; i < w; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// Draw vertical line
void ili9488_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    if (h <= 0) return;
    
    // Fast vertical line drawing, set window then batch transfer the same color data
    ili9488_set_window(x, y, x, y + h - 1);
    
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    for (uint16_t i = 0; i < h; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// Draw horizontal line (24-bit RGB color)
void ili9488_draw_hline_rgb24(uint16_t x, uint16_t y, uint16_t w, uint32_t color24) {
    if (w <= 0) return;
    
    // Fast horizontal line drawing, set window then batch transfer the same color data
    ili9488_set_window(x, y, x + w - 1, y);
    
    uint8_t r, g, b;
    rgb24_to_rgb666(color24, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    for (uint16_t i = 0; i < w; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// Draw vertical line (24-bit RGB color)
void ili9488_draw_vline_rgb24(uint16_t x, uint16_t y, uint16_t h, uint32_t color24) {
    if (h <= 0) return;
    
    // Fast vertical line drawing, set window then batch transfer the same color data
    ili9488_set_window(x, y, x, y + h - 1);
    
    uint8_t r, g, b;
    rgb24_to_rgb666(color24, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    for (uint16_t i = 0; i < h; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// Draw arbitrary line (Bresenham algorithm)
void ili9488_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    // Check special cases (horizontal and vertical lines) for efficiency
    if (y0 == y1) {
        if (x0 > x1) {
            uint16_t temp = x0;
            x0 = x1;
            x1 = temp;
        }
        ili9488_draw_hline(x0, y0, x1 - x0 + 1, color);
        return;
    }
    
    if (x0 == x1) {
        if (y0 > y1) {
            uint16_t temp = y0;
            y0 = y1;
            y1 = temp;
        }
        ili9488_draw_vline(x0, y0, y1 - y0 + 1, color);
        return;
    }
    
    // Use Bresenham algorithm to draw sloped lines
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        // If the line is steeper, swap x and y coordinates
        uint16_t temp = x0;
        x0 = y0;
        y0 = temp;
        
        temp = x1;
        x1 = y1;
        y1 = temp;
    }
    
    if (x0 > x1) {
        // Ensure always drawing from left to right
        uint16_t temp = x0;
        x0 = x1;
        x1 = temp;
        
        temp = y0;
        y0 = y1;
        y1 = temp;
    }
    
    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep;
    
    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }
    
    for (; x0 <= x1; x0++) {
        if (steep) {
            ili9488_draw_pixel(y0, x0, color);
        } else {
            ili9488_draw_pixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

// Draw rectangle
void ili9488_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // Draw four edges
    ili9488_draw_hline(x, y, w, color);          // Top edge
    ili9488_draw_hline(x, y + h - 1, w, color);  // Bottom edge
    ili9488_draw_vline(x, y, h, color);          // Left edge
    ili9488_draw_vline(x + w - 1, y, h, color);  // Right edge
}

// Draw filled rectangle
void ili9488_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // Set window
    ili9488_set_window(x, y, x + w - 1, y + h - 1);
    
    // Prepare color data
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    uint32_t total_pixels = (uint32_t)w * h;
    
    // Fill rectangle
    for (uint32_t i = 0; i < total_pixels; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// Draw filled rectangle (24-bit RGB color)
void ili9488_fill_rect_rgb24(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color24) {
    // Set window
    ili9488_set_window(x, y, x + w - 1, y + h - 1);
    
    // Prepare color data
    uint8_t r, g, b;
    rgb24_to_rgb666(color24, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    uint32_t total_pixels = (uint32_t)w * h;
    
    // Fill rectangle
    for (uint32_t i = 0; i < total_pixels; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// Draw circle
void ili9488_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    // Draw points in eight directions
    ili9488_draw_pixel(x0, y0 + r, color);
    ili9488_draw_pixel(x0, y0 - r, color);
    ili9488_draw_pixel(x0 + r, y0, color);
    ili9488_draw_pixel(x0 - r, y0, color);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        ili9488_draw_pixel(x0 + x, y0 + y, color);
        ili9488_draw_pixel(x0 - x, y0 + y, color);
        ili9488_draw_pixel(x0 + x, y0 - y, color);
        ili9488_draw_pixel(x0 - x, y0 - y, color);
        ili9488_draw_pixel(x0 + y, y0 + x, color);
        ili9488_draw_pixel(x0 - y, y0 + x, color);
        ili9488_draw_pixel(x0 + y, y0 - x, color);
        ili9488_draw_pixel(x0 - y, y0 - x, color);
    }
}

// Draw filled circle
void ili9488_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    // Draw vertical line from center in up and down directions
    ili9488_draw_vline(x0, y0 - r, 2 * r + 1, color);
    ili9488_fill_circle_helper(x0, y0, r, 3, 0, color);
}

// Circle filling helper function
static void ili9488_fill_circle_helper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t corners, uint16_t delta, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;
    
    delta++; // Avoid some rounding errors
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        // These checks avoid drawing extra pixels in quarter circle sections
        if (x < (y + 1)) {
            if (corners & 1) ili9488_draw_hline(x0 + x, y0 - y, 2 * y + 1 - delta, color);
            if (corners & 2) ili9488_draw_hline(x0 - x, y0 - y, 2 * y + 1 - delta, color);
        }
        if (y != py) {
            if (corners & 1) ili9488_draw_hline(x0 + py, y0 - px, 2 * px + 1 - delta, color);
            if (corners & 2) ili9488_draw_hline(x0 - py, y0 - px, 2 * px + 1 - delta, color);
            py = y;
        }
        px = x;
    }
}

// Helper method declaration
static void swap(uint16_t *a, uint16_t *b) {
    uint16_t t = *a;
    *a = *b;
    *b = t;
}

// Draw triangle
void ili9488_draw_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    // Draw three edges
    ili9488_draw_line(x0, y0, x1, y1, color);
    ili9488_draw_line(x1, y1, x2, y2, color);
    ili9488_draw_line(x2, y2, x0, y0, color);
}

// Draw filled triangle
void ili9488_fill_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    int16_t a, b, y, last;
    
    // Sort points by y coordinate
    if (y0 > y1) {
        swap(&y0, &y1);
        swap(&x0, &x1);
    }
    if (y1 > y2) {
        swap(&y1, &y2);
        swap(&x1, &x2);
    }
    if (y0 > y1) {
        swap(&y0, &y1);
        swap(&x0, &x1);
    }
    
    // If all points are on the same horizontal line
    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1;
        else if (x1 > b) b = x1;
        if (x2 < a) a = x2;
        else if (x2 > b) b = x2;
        ili9488_draw_hline(a, y0, b - a + 1, color);
        return;
    }
    
    // To handle various cases of filling triangles, calculate slopes
    int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
    dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;
    
    // For the upper half of the triangle
    if (y1 == y2) last = y1;
    else last = y1 - 1;
    
    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if (a > b) swap(&a, &b);
        ili9488_draw_hline(a, y, b - a + 1, color);
    }
    
    // For the lower half of the triangle
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if (a > b) swap(&a, &b);
        ili9488_draw_hline(a, y, b - a + 1, color);
    }
} 