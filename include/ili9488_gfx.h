/**
 * @file ili9488_gfx.h
 * @brief ILI9488 LCD 图形函数库
 */

#ifndef _ILI9488_GFX_H_
#define _ILI9488_GFX_H_

#include <stdint.h>
#include <stdbool.h>
#include "ili9488.h"

/**
 * @brief 绘制水平线
 * 
 * @param x 起始 X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param color 颜色
 */
void ili9488_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);

/**
 * @brief 绘制水平线（24位RGB颜色）
 * 
 * @param x 起始 X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param color24 24位RGB颜色
 */
void ili9488_draw_hline_rgb24(uint16_t x, uint16_t y, uint16_t w, uint32_t color24);

/**
 * @brief 绘制垂直线
 * 
 * @param x X 坐标
 * @param y 起始 Y 坐标
 * @param h 高度
 * @param color 颜色
 */
void ili9488_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);

/**
 * @brief 绘制垂直线（24位RGB颜色）
 * 
 * @param x X 坐标
 * @param y 起始 Y 坐标
 * @param h 高度
 * @param color24 24位RGB颜色
 */
void ili9488_draw_vline_rgb24(uint16_t x, uint16_t y, uint16_t h, uint32_t color24);

/**
 * @brief 绘制线段
 * 
 * @param x0 起始点 X 坐标
 * @param y0 起始点 Y 坐标
 * @param x1 终点 X 坐标
 * @param y1 终点 Y 坐标
 * @param color 颜色
 */
void ili9488_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief 绘制矩形
 * 
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param w 矩形宽度
 * @param h 矩形高度
 * @param color 颜色
 */
void ili9488_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief 绘制填充矩形
 * 
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param w 矩形宽度
 * @param h 矩形高度
 * @param color 颜色
 */
void ili9488_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief 绘制填充矩形（24位RGB颜色）
 * 
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param w 矩形宽度
 * @param h 矩形高度
 * @param color24 24位RGB颜色
 */
void ili9488_fill_rect_rgb24(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color24);

/**
 * @brief 绘制圆形
 * 
 * @param x0 圆心 X 坐标
 * @param y0 圆心 Y 坐标
 * @param r 半径
 * @param color 颜色
 */
void ili9488_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief 绘制填充圆形
 * 
 * @param x0 圆心 X 坐标
 * @param y0 圆心 Y 坐标
 * @param r 半径
 * @param color 颜色
 */
void ili9488_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief 绘制三角形
 * 
 * @param x0 第一个顶点 X 坐标
 * @param y0 第一个顶点 Y 坐标
 * @param x1 第二个顶点 X 坐标
 * @param y1 第二个顶点 Y 坐标
 * @param x2 第三个顶点 X 坐标
 * @param y2 第三个顶点 Y 坐标
 * @param color 颜色
 */
void ili9488_draw_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief 绘制填充三角形
 * 
 * @param x0 第一个顶点 X 坐标
 * @param y0 第一个顶点 Y 坐标
 * @param x1 第二个顶点 X 坐标
 * @param y1 第二个顶点 Y 坐标
 * @param x2 第三个顶点 X 坐标
 * @param y2 第三个顶点 Y 坐标
 * @param color 颜色
 */
void ili9488_fill_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief 绘制字符
 * 
 * @param x 起始 X 坐标
 * @param y 起始 Y 坐标
 * @param c 字符
 * @param color 文字颜色
 * @param bg 背景颜色
 * @param size 字体大小
 */
void ili9488_draw_char(uint16_t x, uint16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief 绘制字符串
 * 
 * @param x 起始 X 坐标
 * @param y 起始 Y 坐标
 * @param str 字符串
 * @param color 文字颜色
 * @param bg 背景颜色
 * @param size 字体大小
 */
void ili9488_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief 绘制位图
 * 
 * @param x 起始 X 坐标
 * @param y 起始 Y 坐标
 * @param width 位图宽度
 * @param height 位图高度
 * @param bitmap 位图数据 (RGB565 格式)
 */
void ili9488_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *bitmap);

/**
 * @brief 绘制中文字符
 * 
 * @param x 起始 X 坐标
 * @param y 起始 Y 坐标
 * @param index 字符索引
 * @param color 颜色
 * @param font_data 字体数据指针
 */
void ili9488_draw_chinese(uint16_t x, uint16_t y, uint8_t index, uint16_t color, const unsigned char (*font_data)[32]);

#endif /* _ILI9488_GFX_H_ */ 