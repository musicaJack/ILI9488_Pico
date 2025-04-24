/**
 * @file ili9488_gfx.c
 * @brief ILI9488 LCD 图形函数库实现
 */

#include <stdlib.h>
#include "ili9488_gfx.h"
#include "ili9488.h"

// 将RGB565颜色转换为RGB666格式（18位）- 在这里声明
static void rgb565_to_rgb666(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
    // 从RGB565中提取分量
    *r = (color >> 11) & 0x1F;  // 5位红色
    *g = (color >> 5) & 0x3F;   // 6位绿色
    *b = color & 0x1F;          // 5位蓝色
    
    // 扩展到RGB666格式（每个颜色分量占用6位）
    *r = (*r << 1) | (*r >> 4); // 将5位红色扩展到6位
    // 绿色已经是6位，不需要扩展
    *b = (*b << 1) | (*b >> 4); // 将5位蓝色扩展到6位
}

// 从24位RGB颜色提取RGB分量并转换为RGB666格式
static void rgb24_to_rgb666(uint32_t color24, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (color24 >> 16) & 0xFF;  // 提取R分量（高8位）
    *g = (color24 >> 8) & 0xFF;   // 提取G分量（中8位）
    *b = color24 & 0xFF;          // 提取B分量（低8位）
    
    // 将8位分量转换为6位分量
    *r = *r >> 2;  // 8位到6位
    *g = *g >> 2;  // 8位到6位
    *b = *b >> 2;  // 8位到6位
}

// 填充圆形辅助函数的声明
static void ili9488_fill_circle_helper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t corners, uint16_t delta, uint16_t color);

// 绘制水平线
void ili9488_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    if (w <= 0) return;
    
    // 快速水平线绘制，设置窗口然后批量传输相同的颜色数据
    ili9488_set_window(x, y, x + w - 1, y);
    
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    for (uint16_t i = 0; i < w; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// 绘制垂直线
void ili9488_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    if (h <= 0) return;
    
    // 快速垂直线绘制，设置窗口然后批量传输相同的颜色数据
    ili9488_set_window(x, y, x, y + h - 1);
    
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    for (uint16_t i = 0; i < h; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// 绘制水平线（24位RGB颜色）
void ili9488_draw_hline_rgb24(uint16_t x, uint16_t y, uint16_t w, uint32_t color24) {
    if (w <= 0) return;
    
    // 快速水平线绘制，设置窗口然后批量传输相同的颜色数据
    ili9488_set_window(x, y, x + w - 1, y);
    
    uint8_t r, g, b;
    rgb24_to_rgb666(color24, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    for (uint16_t i = 0; i < w; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// 绘制垂直线（24位RGB颜色）
void ili9488_draw_vline_rgb24(uint16_t x, uint16_t y, uint16_t h, uint32_t color24) {
    if (h <= 0) return;
    
    // 快速垂直线绘制，设置窗口然后批量传输相同的颜色数据
    ili9488_set_window(x, y, x, y + h - 1);
    
    uint8_t r, g, b;
    rgb24_to_rgb666(color24, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    for (uint16_t i = 0; i < h; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// 绘制任意线段（Bresenham算法）
void ili9488_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    // 检查特殊情况（水平线和垂直线）提高效率
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
    
    // 使用Bresenham算法绘制斜线
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        // 如果线段更陡峭，交换x和y坐标
        uint16_t temp = x0;
        x0 = y0;
        y0 = temp;
        
        temp = x1;
        x1 = y1;
        y1 = temp;
    }
    
    if (x0 > x1) {
        // 确保总是从左向右绘制
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

// 绘制矩形
void ili9488_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // 绘制四条边
    ili9488_draw_hline(x, y, w, color);          // 上边
    ili9488_draw_hline(x, y + h - 1, w, color);  // 下边
    ili9488_draw_vline(x, y, h, color);          // 左边
    ili9488_draw_vline(x + w - 1, y, h, color);  // 右边
}

// 绘制填充矩形
void ili9488_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // 设置窗口
    ili9488_set_window(x, y, x + w - 1, y + h - 1);
    
    // 准备颜色数据
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    uint32_t total_pixels = (uint32_t)w * h;
    
    // 填充矩形
    for (uint32_t i = 0; i < total_pixels; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// 绘制填充矩形（24位RGB颜色）
void ili9488_fill_rect_rgb24(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color24) {
    // 设置窗口
    ili9488_set_window(x, y, x + w - 1, y + h - 1);
    
    // 准备颜色数据
    uint8_t r, g, b;
    rgb24_to_rgb666(color24, &r, &g, &b);
    
    uint8_t data[3] = {r, g, b};
    uint32_t total_pixels = (uint32_t)w * h;
    
    // 填充矩形
    for (uint32_t i = 0; i < total_pixels; i++) {
        ili9488_write_data_buffer(data, 3);
    }
}

// 绘制圆形
void ili9488_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    // 绘制八个方向的点
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

// 绘制填充圆形
void ili9488_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    // 绘制垂直线从圆心向上下方向填充
    ili9488_draw_vline(x0, y0 - r, 2 * r + 1, color);
    ili9488_fill_circle_helper(x0, y0, r, 3, 0, color);
}

// 填充圆形辅助函数
static void ili9488_fill_circle_helper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t corners, uint16_t delta, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;
    
    delta++; // 避免一些舍入误差
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        // 这些检查避免在四分之一圆环节绘制多余的像素
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

// 辅助方法声明
static void swap(uint16_t *a, uint16_t *b) {
    uint16_t t = *a;
    *a = *b;
    *b = t;
}

// 绘制三角形
void ili9488_draw_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    // 绘制三条边
    ili9488_draw_line(x0, y0, x1, y1, color);
    ili9488_draw_line(x1, y1, x2, y2, color);
    ili9488_draw_line(x2, y2, x0, y0, color);
}

// 绘制填充三角形
void ili9488_fill_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    int16_t a, b, y, last;
    
    // 按y坐标排序点
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
    
    // 如果全部点在同一水平线上
    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1;
        else if (x1 > b) b = x1;
        if (x2 < a) a = x2;
        else if (x2 > b) b = x2;
        ili9488_draw_hline(a, y0, b - a + 1, color);
        return;
    }
    
    // 为了处理填充三角形的各种情况，计算斜率
    int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
    dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;
    
    // 对于上半部分三角形
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
    
    // 对于下半部分三角形
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