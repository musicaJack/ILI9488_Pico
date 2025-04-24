/**
 * @file ili9488.h
 * @brief ILI9488 LCD驱动头文件
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
 * @brief LCD配置结构体
 */
typedef struct {
    spi_inst_t *spi_inst;    // SPI实例
    uint32_t spi_speed_hz;   // SPI速度(Hz)
    
    // 引脚定义
    uint8_t pin_din;         // MOSI引脚
    uint8_t pin_sck;         // SCK引脚
    uint8_t pin_cs;          // CS引脚
    uint8_t pin_dc;          // 数据/命令引脚
    uint8_t pin_reset;       // 复位引脚
    uint8_t pin_bl;          // 背光引脚
    
    // 屏幕参数
    uint16_t width;          // 宽度
    uint16_t height;         // 高度
    
    // 方向
    uint8_t rotation;        // 旋转方向
} ili9488_config_t;

/**
 * @brief 颜色定义 (RGB666格式，按厂商标准)
 */
#define ILI9488_BLACK       0x000000  // 黑色
#define ILI9488_WHITE       0xFCFCFC  // 白色
#define ILI9488_RED         0xFC0000  // 红色
#define ILI9488_GREEN       0x00FC00  // 绿色
#define ILI9488_BLUE        0x0000FC  // 蓝色
#define ILI9488_YELLOW      0xFCFC00  // 黄色
#define ILI9488_CYAN        0x00FCFC  // 青色
#define ILI9488_MAGENTA     0xFC00FC  // 品红色

/* ILI9488命令定义 */
#define ILI9488_NOP        0x00     // 空操作
#define ILI9488_SWRESET    0x01     // 软件复位
#define ILI9488_RDDID      0x04     // 读显示器ID
#define ILI9488_SLPIN      0x10     // 进入睡眠模式
#define ILI9488_SLPOUT     0x11     // 退出睡眠模式
#define ILI9488_PTLON      0x12     // 部分模式开
#define ILI9488_NORON      0x13     // 正常显示模式开
#define ILI9488_INVOFF     0x20     // 关闭显示反转
#define ILI9488_INVON      0x21     // 打开显示反转
#define ILI9488_DISPOFF    0x28     // 关闭显示
#define ILI9488_DISPON     0x29     // 打开显示
#define ILI9488_CASET      0x2A     // 列地址设置
#define ILI9488_PASET      0x2B     // 页(行)地址设置
#define ILI9488_RAMWR      0x2C     // 内存写
#define ILI9488_RAMRD      0x2E     // 内存读
#define ILI9488_PTLAR      0x30     // 部分区域
#define ILI9488_VSCRDEF    0x33     // 垂直滚动定义
#define ILI9488_MADCTL     0x36     // 内存访问控制
#define ILI9488_VSCRSADD   0x37     // 垂直滚动起始地址
#define ILI9488_PIXFMT     0x3A     // 接口像素格式

// MADCTL位定义
#define ILI9488_MADCTL_MY  0x80     // 行地址顺序选择(0=从上到下,1=从下到上)
#define ILI9488_MADCTL_MX  0x40     // 列地址顺序选择(0=从左到右,1=从右到左)
#define ILI9488_MADCTL_MV  0x20     // 行列交换(0=正常,1=交换)
#define ILI9488_MADCTL_ML  0x10     // 垂直更新顺序(0=从上到下,1=从下到上)
#define ILI9488_MADCTL_BGR 0x08     // BGR/RGB顺序(0=RGB,1=BGR)
#define ILI9488_MADCTL_MH  0x04     // 水平更新顺序(0=从左到右,1=从右到左)

/**
 * @brief 初始化ILI9488驱动
 * 
 * @param config LCD配置参数
 * @return bool 初始化是否成功
 */
bool ili9488_init(const ili9488_config_t *config);

/**
 * @brief 设置LCD背光
 * 
 * @param on 背光状态(true为开,false为关)
 */
void ili9488_set_backlight(bool on);

/**
 * @brief 设置LCD背光亮度
 * 
 * @param brightness 亮度等级(0-255)，0为关闭，255为最亮
 */
void ili9488_set_backlight_brightness(uint8_t brightness);

/**
 * @brief 用指定颜色填充整个屏幕
 * 
 * @param color 填充颜色(RGB565格式)
 */
void ili9488_fill_screen(uint16_t color);

/**
 * @brief 设置绘图窗口
 * 
 * @param x0 起始X坐标
 * @param y0 起始Y坐标
 * @param x1 结束X坐标
 * @param y1 结束Y坐标
 */
void ili9488_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief 绘制单个像素
 * 
 * @param x X坐标
 * @param y Y坐标
 * @param color 像素颜色
 */
void ili9488_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief 绘制单个像素（24位RGB格式）
 * 
 * @param x X坐标
 * @param y Y坐标
 * @param color24 24位RGB颜色
 */
void ili9488_draw_pixel_rgb24(uint16_t x, uint16_t y, uint32_t color24);

/**
 * @brief 向LCD发送数据块
 * 
 * @param data 数据指针
 * @param len 数据长度
 */
void ili9488_write_data_buffer(const uint8_t *data, size_t len);

/**
 * @brief 设置显示方向
 * 
 * @param rotation 旋转值(0-3)
 */
void ili9488_set_rotation(uint8_t rotation);

/**
 * @brief 填充屏幕为单一颜色（24位RGB格式）
 * 
 * @param color24 24位RGB颜色
 */
void ili9488_fill_screen_rgb24(uint32_t color24);

#endif /* _ILI9488_H_ */ 