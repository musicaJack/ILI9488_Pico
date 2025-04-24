/**
 * @file ili9488_gfx.h
 * @brief ILI9488 LCD Graphics Library
 */

#ifndef _ILI9488_GFX_H_
#define _ILI9488_GFX_H_

#include <stdint.h>
#include <stdbool.h>
#include "ili9488.h"

/**
 * @brief Draw a horizontal line
 * 
 * @param x Starting X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param color Color
 */
void ili9488_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);

/**
 * @brief Draw a horizontal line (24-bit RGB color)
 * 
 * @param x Starting X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param color24 24-bit RGB color
 */
void ili9488_draw_hline_rgb24(uint16_t x, uint16_t y, uint16_t w, uint32_t color24);

/**
 * @brief Draw a vertical line
 * 
 * @param x X coordinate
 * @param y Starting Y coordinate
 * @param h Height
 * @param color Color
 */
void ili9488_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);

/**
 * @brief Draw a vertical line (24-bit RGB color)
 * 
 * @param x X coordinate
 * @param y Starting Y coordinate
 * @param h Height
 * @param color24 24-bit RGB color
 */
void ili9488_draw_vline_rgb24(uint16_t x, uint16_t y, uint16_t h, uint32_t color24);

/**
 * @brief Draw a line segment
 * 
 * @param x0 Starting point X coordinate
 * @param y0 Starting point Y coordinate
 * @param x1 End point X coordinate
 * @param y1 End point Y coordinate
 * @param color Color
 */
void ili9488_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief Draw a rectangle
 * 
 * @param x Top-left corner X coordinate
 * @param y Top-left corner Y coordinate
 * @param w Rectangle width
 * @param h Rectangle height
 * @param color Color
 */
void ili9488_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Draw a filled rectangle
 * 
 * @param x Top-left corner X coordinate
 * @param y Top-left corner Y coordinate
 * @param w Rectangle width
 * @param h Rectangle height
 * @param color Color
 */
void ili9488_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Draw a filled rectangle (24-bit RGB color)
 * 
 * @param x Top-left corner X coordinate
 * @param y Top-left corner Y coordinate
 * @param w Rectangle width
 * @param h Rectangle height
 * @param color24 24-bit RGB color
 */
void ili9488_fill_rect_rgb24(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color24);

/**
 * @brief Draw a circle
 * 
 * @param x0 Center X coordinate
 * @param y0 Center Y coordinate
 * @param r Radius
 * @param color Color
 */
void ili9488_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief Draw a filled circle
 * 
 * @param x0 Center X coordinate
 * @param y0 Center Y coordinate
 * @param r Radius
 * @param color Color
 */
void ili9488_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief Draw a triangle
 * 
 * @param x0 First vertex X coordinate
 * @param y0 First vertex Y coordinate
 * @param x1 Second vertex X coordinate
 * @param y1 Second vertex Y coordinate
 * @param x2 Third vertex X coordinate
 * @param y2 Third vertex Y coordinate
 * @param color Color
 */
void ili9488_draw_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief Draw a filled triangle
 * 
 * @param x0 First vertex X coordinate
 * @param y0 First vertex Y coordinate
 * @param x1 Second vertex X coordinate
 * @param y1 Second vertex Y coordinate
 * @param x2 Third vertex X coordinate
 * @param y2 Third vertex Y coordinate
 * @param color Color
 */
void ili9488_fill_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief Draw a character
 * 
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param c Character
 * @param color Text color
 * @param bg Background color
 * @param size Font size
 */
void ili9488_draw_char(uint16_t x, uint16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief Draw a string
 * 
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param str String
 * @param color Text color
 * @param bg Background color
 * @param size Font size
 */
void ili9488_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief Draw a bitmap
 * 
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param width Bitmap width
 * @param height Bitmap height
 * @param bitmap Bitmap data (RGB565 format)
 */
void ili9488_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *bitmap);

/**
 * @brief Draw a Chinese character
 * 
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param index Character index
 * @param color Color
 * @param font_data Font data pointer
 */
void ili9488_draw_chinese(uint16_t x, uint16_t y, uint8_t index, uint16_t color, const unsigned char (*font_data)[32]);

#endif /* _ILI9488_GFX_H_ */ 