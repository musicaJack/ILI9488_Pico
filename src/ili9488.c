/**
 * @file ili9488.c
 * @brief ILI9488 LCD驱动实现
 */

#include <stdio.h>
#include <string.h>
#include "ili9488.h"
#include "ili9488_hal.h"

// 全局变量，保存LCD配置信息
static struct {
    bool is_initialized;
    uint16_t width;
    uint16_t height;
    uint8_t rotation;
} ili9488 = {0};

// 根据厂商示例代码实现的ILI9488初始化序列
static void ili9488_init_sequence(void) {
    printf("执行ILI9488初始化序列...\n");
    
    // 软件复位
    ili9488_hal_write_cmd(ILI9488_SWRESET);
    ili9488_hal_delay_ms(200);

    // 退出睡眠
    ili9488_hal_write_cmd(ILI9488_SLPOUT);
    ili9488_hal_delay_ms(200);
    
    // 设置内存访问控制（MADCTL）
    ili9488_hal_write_cmd(ILI9488_MADCTL);
    ili9488_hal_write_data(0x48); // 该指令定义帧存储器的读写扫描方向
    
    // 设置像素格式 - 根据GMT035-04-4SPI.ino示例使用18位色
    ili9488_hal_write_cmd(ILI9488_PIXFMT);
    ili9488_hal_write_data(0x66); // 18位/像素 (666格式)
    
    // VCOM控制
    ili9488_hal_write_cmd(0xC5);
    ili9488_hal_write_data(0x00);
    ili9488_hal_write_data(0x36);
    ili9488_hal_write_data(0x80);
    
    // 电源控制
    ili9488_hal_write_cmd(0xC2);
    ili9488_hal_write_data(0xA7);
    
    // 正极伽马校准
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
    
    // 负极伽马校准
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
    
    // 反转显示
    ili9488_hal_write_cmd(ILI9488_INVON);
    
    // 开显示
    ili9488_hal_write_cmd(ILI9488_DISPON);
    
    ili9488_hal_delay_ms(50);
}

// 初始化ILI9488
bool ili9488_init(const ili9488_config_t *config) {
    printf("初始化ILI9488 LCD...\n");
    
    if (ili9488.is_initialized) {
        printf("ILI9488已经初始化\n");
        return true;
    }
    
    if (config == NULL) {
        printf("错误：配置为空\n");
        return false;
    }
    
    // 保存LCD屏幕参数
    ili9488.width = config->width;
    ili9488.height = config->height;
    ili9488.rotation = config->rotation;
    
    // 设置硬件抽象层配置
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
    
    // 初始化硬件抽象层
    if (!ili9488_hal_init(&hw_config)) {
        printf("错误：硬件初始化失败\n");
        return false;
    }
    
    // 执行硬件复位
    ili9488_hal_reset();
    
    // 运行初始化序列
    ili9488_init_sequence();
    
    // 设置旋转方向
    ili9488_set_rotation(ili9488.rotation);
    
    ili9488.is_initialized = true;
    printf("ILI9488初始化完成\n");
    
    return true;
}

// 设置LCD背光
void ili9488_set_backlight(bool on) {
    ili9488_hal_set_backlight(on);
}

// 设置LCD背光亮度
void ili9488_set_backlight_brightness(uint8_t brightness) {
    ili9488_hal_set_backlight_brightness(brightness);
}

// 设置绘图窗口
void ili9488_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // 设置列地址
    ili9488_hal_write_cmd(ILI9488_CASET);
    ili9488_hal_write_data(x0 >> 8);
    ili9488_hal_write_data(x0 & 0xFF);
    ili9488_hal_write_data(x1 >> 8);
    ili9488_hal_write_data(x1 & 0xFF);
    
    // 设置行地址
    ili9488_hal_write_cmd(ILI9488_PASET);
    ili9488_hal_write_data(y0 >> 8);
    ili9488_hal_write_data(y0 & 0xFF);
    ili9488_hal_write_data(y1 >> 8);
    ili9488_hal_write_data(y1 & 0xFF);
    
    // 准备写入数据
    ili9488_hal_write_cmd(ILI9488_RAMWR);
}

// 将RGB565颜色转换为RGB666格式（18位） - 对ILI9488必要
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

// 将24位RGB颜色转换为RGB666格式 - 新增，根据厂商标准
static void rgb24_to_rgb666(uint32_t color24, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (color24 >> 16) & 0xFF;  // 提取R分量（高8位）
    *g = (color24 >> 8) & 0xFF;   // 提取G分量（中8位）
    *b = color24 & 0xFF;          // 提取B分量（低8位）
    
    // 将8位分量转换为6位分量
    *r = *r >> 2;  // 8位到6位
    *g = *g >> 2;  // 8位到6位
    *b = *b >> 2;  // 8位到6位
}

// 绘制单个像素
void ili9488_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= ili9488.width || y >= ili9488.height) {
        return;  // 超出显示范围
    }
    
    ili9488_set_window(x, y, x, y);
    
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    // 发送RGB666格式数据（三字节）
    uint8_t data[3] = {r, g, b};
    ili9488_hal_write_data_buffer(data, 3);
}

// 绘制单个像素（24位RGB格式）- 新增
void ili9488_draw_pixel_rgb24(uint16_t x, uint16_t y, uint32_t color24) {
    if (x >= ili9488.width || y >= ili9488.height) {
        return;  // 超出显示范围
    }
    
    ili9488_set_window(x, y, x, y);
    
    uint8_t r, g, b;
    rgb24_to_rgb666(color24, &r, &g, &b);
    
    // 发送RGB666格式数据（三字节）
    uint8_t data[3] = {r, g, b};
    ili9488_hal_write_data_buffer(data, 3);
}

// 向LCD发送数据缓冲区
void ili9488_write_data_buffer(const uint8_t *data, size_t len) {
    ili9488_hal_write_data_buffer(data, len);
}

// 填充屏幕为单一颜色
void ili9488_fill_screen(uint16_t color) {
    uint8_t r, g, b;
    rgb565_to_rgb666(color, &r, &g, &b);
    
    // 设置绘图区域为整个屏幕
    ili9488_set_window(0, 0, ili9488.width - 1, ili9488.height - 1);
    
    // 准备数据
    uint8_t data[3] = {r, g, b};
    
    // 逐像素填充
    for (uint32_t i = 0; i < (uint32_t)ili9488.width * ili9488.height; i++) {
        ili9488_hal_write_data_buffer(data, 3);
    }
}

// 填充屏幕为单一颜色（24位RGB格式）- 新增
void ili9488_fill_screen_rgb24(uint32_t color24) {
    uint8_t r, g, b;
    rgb24_to_rgb666(color24, &r, &g, &b);
    
    // 设置绘图区域为整个屏幕
    ili9488_set_window(0, 0, ili9488.width - 1, ili9488.height - 1);
    
    // 准备数据
    uint8_t data[3] = {r, g, b};
    
    // 逐像素填充
    for (uint32_t i = 0; i < (uint32_t)ili9488.width * ili9488.height; i++) {
        ili9488_hal_write_data_buffer(data, 3);
    }
}

// 旋转映射表
static const uint8_t ili9488_rotation_map[4] = {
    0x48, // 0度旋转: MY(1) | BGR(1)
    0x28, // 90度旋转: MV(1) | BGR(1)
    0x88, // 180度旋转: MX(1) | MY(1) | BGR(1)
    0xE8  // 270度旋转: MX(1) | MY(1) | MV(1) | BGR(1)
};

// 设置显示方向
void ili9488_set_rotation(uint8_t rotation) {
    rotation = rotation % 4;  // 范围限制为0-3
    ili9488.rotation = rotation;
    
    // 修改MADCTL寄存器
    ili9488_hal_write_cmd(ILI9488_MADCTL);
    ili9488_hal_write_data(ili9488_rotation_map[rotation]);
    
    // 调整宽度和高度
    if (rotation == 1 || rotation == 3) {
        // 横向模式
        ili9488.width = 480;  // ILI9488原始高度
        ili9488.height = 320; // ILI9488原始宽度
    } else {
        // 纵向模式
        ili9488.width = 320;  // ILI9488原始宽度
        ili9488.height = 480; // ILI9488原始高度
    }
} 