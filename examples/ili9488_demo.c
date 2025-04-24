/**
 * @file ili9488_demo.c
 * @brief ILI9488 LCD 驱动演示程序
 */

#include <stdio.h>
#include <string.h>  // 添加string.h头文件，提供strlen函数声明
#include "pico/stdlib.h"
#include "ili9488.h"
#include "ili9488_gfx.h"
#include <math.h> // 为sin函数添加头文件

// 引脚定义 - 保持与原有 lcd_demo.c 一致
#define PIN_DIN   19  // SPI MOSI
#define PIN_SCK   18  // SPI SCK
#define PIN_CS    17  // SPI CS - 手动控制
#define PIN_DC    20  // 数据/命令
#define PIN_RESET 15  // 复位
#define PIN_BL    10  // 背光

// 屏幕参数 - ILI9488
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 480

// 参照厂商代码的颜色定义（24位RGB格式）- 使用ILI9488中定义的常量
#define COLOR_RED     ILI9488_RED     // 红色
#define COLOR_GREEN   ILI9488_GREEN   // 绿色
#define COLOR_BLUE    ILI9488_BLUE    // 蓝色
#define COLOR_WHITE   ILI9488_WHITE   // 白色
#define COLOR_BLACK   ILI9488_BLACK   // 黑色
#define COLOR_YELLOW  0xFCFC00        // 黄色 (红+绿)
#define COLOR_CYAN    0x00FCFC        // 青色 (绿+蓝)
#define COLOR_MAGENTA 0xFC00FC        // 品红 (红+蓝)
#define COLOR_ORANGE  0xFC7800        // 橙色
#define COLOR_PURPLE  0x7800FC        // 紫色
#define COLOR_LIME    0x78FC00        // 酸橙色

// 函数前向声明
static void demo_static_graphics(void);
static void demo_color_animation(void);
static void demo_color_test(void);
static void demo_gradient_transition(void);
static void demo_brightness_checkboard(void);
static void demo_poetry(void);
static void ili9488_draw_centered_string(const char* str, uint16_t y, uint32_t color, uint32_t bgColor, uint8_t size);

// 辅助函数 - 创建RGB565颜色（保留用于兼容性）
static inline uint16_t ili9488_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

// 辅助函数 - 创建RGB666颜色（按照厂商代码设计）
// 注意：这个函数返回24位颜色值，高6位R，中间6位G，低6位B
static inline uint32_t ili9488_color_666(uint8_t r, uint8_t g, uint8_t b) {
    // 限制在6位范围内（0-63）
    r &= 0x3F;
    g &= 0x3F;
    b &= 0x3F;
    
    // 转换为24位颜色值（每个通道左移2位，与厂商代码保持一致）
    return ((uint32_t)(r << 2) << 16) | ((uint32_t)(g << 2) << 8) | (b << 2);
}

// 辅助函数 - 从24位RGB颜色提取RGB分量并转换为RGB666格式
static void RGB24_to_RGB666(uint32_t color24, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (color24 >> 16) & 0xFF;  // 提取R分量（高8位）
    *g = (color24 >> 8) & 0xFF;   // 提取G分量（中8位）
    *b = color24 & 0xFF;          // 提取B分量（低8位）
    
    // 将8位分量转换为6位分量
    *r = *r >> 2;  // 8位到6位
    *g = *g >> 2;  // 8位到6位
    *b = *b >> 2;  // 8位到6位
}

// 辅助函数 - 使用24位RGB颜色绘制字符串
static void ili9488_draw_string_rgb24(uint16_t x, uint16_t y, const char *str, uint32_t color, uint32_t bg, uint8_t size) {
    uint8_t r, g, b, bg_r, bg_g, bg_b;
    
    // 从24位RGB颜色提取RGB分量
    RGB24_to_RGB666(color, &r, &g, &b);
    RGB24_to_RGB666(bg, &bg_r, &bg_g, &bg_b);
    
    // 转换为RGB565格式（兼容现有的绘制字符串函数）
    uint16_t color565 = ili9488_color(r >> 1, g, b >> 1); // 将6位缩减为5位
    uint16_t bg565 = ili9488_color(bg_r >> 1, bg_g, bg_b >> 1);
    
    // 调用现有的绘制字符串函数
    ili9488_draw_string(x, y, str, color565, bg565, size);
}

// 静态文字演示
static void demo_static_text(void) {
    printf("执行静态图形演示...\n");
    // 旋转屏幕90度
    ili9488_set_rotation(1);
    // 打开背光并立即设置为最高亮度
    ili9488_set_backlight_brightness(255);
    // 填充整个屏幕为黑色
    ili9488_fill_screen_rgb24(COLOR_BLUE);
    
    // 绘制文字 - 白色文字，黑色背景
    printf("绘制文字...\n");
    ili9488_draw_string_rgb24(30, 80, "Satellites whisper,", ILI9488_WHITE, ILI9488_BLUE, 2);
    ili9488_draw_string_rgb24(30, 120, "pixels dance.", ILI9488_WHITE, ILI9488_BLUE, 2);
    ili9488_draw_string_rgb24(30, 160, "Pico brings them", ILI9488_WHITE, ILI9488_BLUE, 2);
    ili9488_draw_string_rgb24(30, 200, "both to life.", ILI9488_WHITE, ILI9488_BLUE, 2);
    sleep_ms(5000);
}


// 颜色动画演示
static void demo_color_animation(void) {
    printf("执行颜色动画演示...\n");
    
    // 设置最高亮度
    ili9488_set_backlight_brightness(255);
    
    uint32_t t = 0;
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t frames = 0;
    
    // 运行动画10秒
    while (to_ms_since_boot(get_absolute_time()) - start_time < 10000) {
        uint8_t r, g, b;
        
        // 设置绘图窗口为整个屏幕
        ili9488_set_window(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
        
        // 绘制动态颜色图案 - 使用正弦函数产生更鲜艳的颜色
        for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
            for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
                // 使用与时间相关的正弦函数生成更鲜艳的颜色
                float phase_r = (float)(x + t * 2) / 128.0f;
                float phase_g = (float)(y - t) / 128.0f;
                float phase_b = (float)(x + y + t) / 256.0f;
                
                // 生成0-63范围的RGB分量(6位)
                r = (uint8_t)(31.5f + 31.5f * sin(phase_r));
                g = (uint8_t)(31.5f + 31.5f * sin(phase_g));
                b = (uint8_t)(31.5f + 31.5f * sin(phase_b));
                
                // 直接使用RGB666格式发送数据
                uint8_t data[3] = {r, g, b};
                ili9488_write_data_buffer(data, 3);
            }
        }
        
        t++;
        frames++;
        
        if (frames % 10 == 0) {
            printf("已完成 %lu 帧\n", frames);
        }
    }
    
    float fps = (float)frames / 10.0f;
    printf("动画完成: %lu 帧, 平均 %.1f FPS\n", frames, fps);
}

// 颜色测试
static void demo_color_test(void) {
    printf("执行颜色测试...\n");
    
    // 亮度设置为最大
    ili9488_set_backlight_brightness(255);
    
    // 使用厂商定义的颜色（24位RGB格式）
    printf("颜色测试开始...\n");
    
    // 红色
    ili9488_fill_screen_rgb24(COLOR_RED);
    printf("显示红色...\n");
    sleep_ms(1000);
    
    // 绿色
    ili9488_fill_screen_rgb24(COLOR_GREEN);
    printf("显示绿色...\n");
    sleep_ms(1000);
    
    // 蓝色
    ili9488_fill_screen_rgb24(COLOR_BLUE);
    printf("显示蓝色...\n");
    sleep_ms(1000);
    
    // 黄色
    ili9488_fill_screen_rgb24(COLOR_YELLOW);
    printf("显示黄色...\n");
    sleep_ms(1000);
    
    // 青色
    ili9488_fill_screen_rgb24(COLOR_CYAN);
    printf("显示青色...\n");
    sleep_ms(1000);
    
    // 品红色
    ili9488_fill_screen_rgb24(COLOR_MAGENTA);
    printf("显示品红色...\n");
    sleep_ms(1000);
    
    // 橙色
    ili9488_fill_screen_rgb24(COLOR_ORANGE);
    printf("显示橙色...\n");
    sleep_ms(1000);
    
    // 紫色
    ili9488_fill_screen_rgb24(COLOR_PURPLE);
    printf("显示紫色...\n");
    sleep_ms(1000);
    
    // 黑色
    ili9488_fill_screen_rgb24(COLOR_BLACK);
    printf("显示黑色...\n");
    sleep_ms(1000);
    
    // 白色
    ili9488_fill_screen_rgb24(COLOR_WHITE);
    printf("显示白色...\n");
    sleep_ms(1000);
}

// 渐变色过渡显示 (新添加)
static void demo_gradient_transition(void) {
    printf("执行渐变色过渡显示...\n");
    
    // 设置最高亮度
    ili9488_set_backlight_brightness(255);
    
    // 计算总帧数 (假设10秒内完成256种颜色过渡，约25帧/秒)
    const uint16_t total_frames = 250; // 略少于256，留出一些余量
    const uint32_t duration_ms = 10000; // 总时长10秒
    const uint32_t frame_time_ms = duration_ms / total_frames;
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t frames = 0;
    
    // 设置绘图窗口为整个屏幕
    ili9488_set_window(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    
    while (frames < total_frames) {
        // 计算当前渐变值 (0-255)
        uint8_t gradient = (frames * 256) / total_frames;
        
        // 创建渐变色 - 使用厂商颜色定义的方式，更鲜艳
        // 从蓝色过渡到红色，经过绿色和其他颜色
        uint8_t r, g, b;
        
        // 使用正弦函数产生更平滑的颜色过渡
        float phase = (float)frames / total_frames * 3.0f; // 三个周期的相位
        
        // 生成0-63范围的RGB分量(6位)
        r = (uint8_t)(31.5f + 31.5f * sin(phase * 3.14159f));
        g = (uint8_t)(31.5f + 31.5f * sin((phase + 0.33f) * 2.0f * 3.14159f));
        b = (uint8_t)(31.5f + 31.5f * sin((phase + 0.66f) * 3.14159f));
        
        // 直接使用RGB666格式的数据
        // 填充整个屏幕
        for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
            for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
                uint8_t data[3] = {r, g, b};
                ili9488_write_data_buffer(data, 3);
            }
        }
        
        frames++;
        
        // 打印进度
        if (frames % 25 == 0) {
            printf("渐变过渡: %.1f%%\n", (float)frames * 100.0f / total_frames);
        }
        
        // 控制帧率
        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - start_time;
        uint32_t target_time = frames * frame_time_ms;
        if (elapsed < target_time) {
            sleep_ms(target_time - elapsed);
        }
    }
    
    printf("渐变色过渡完成\n");
}

// 亮度变化的黑白方格和RGB颜色演示 (新添加)
static void demo_brightness_checkboard(void) {
    printf("执行亮度变化的黑白方格和RGB颜色演示...\n");
    
    // 由于此演示特别演示亮度变化，所以不设置固定亮度
    // 但确保第二阶段RGB颜色部分使用最高亮度
    
    // 总时长20秒，分为两个阶段：黑白变化和RGB变化
    const uint32_t total_duration_ms = 20000;
    const uint32_t bw_duration_ms = 10000; // 黑白阶段10秒
    const uint32_t rgb_duration_ms = 10000; // RGB阶段10秒
    
    const uint8_t grid_size = 20; // 方格大小
    const uint16_t cols = SCREEN_WIDTH / grid_size;
    const uint16_t rows = SCREEN_HEIGHT / grid_size;
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t current_time;
    
    // 阶段1: 黑白亮度变化 (亮度从0到255)
    printf("黑白方格亮度变化...\n");
    uint8_t brightness_steps = 100; // 亮度级别
    for (uint8_t step = 0; step < brightness_steps; step++) {
        // 计算当前亮度
        uint8_t brightness = (step * 255) / (brightness_steps - 1);
        
        // 根据亮度调整黑白方格的颜色，限制在RGB666格式范围内（0-63）
        uint8_t dark_value = 0;
        uint8_t light_value = brightness >> 2; // 将8位亮度转换为6位（0-63）
        if (light_value > 63) light_value = 63; // 确保在范围内
        
        // 绘制黑白方格
        for (uint16_t row = 0; row < rows; row++) {
            for (uint16_t col = 0; col < cols; col++) {
                uint16_t x = col * grid_size;
                uint16_t y = row * grid_size;
                uint8_t value = ((row + col) % 2 == 0) ? light_value : dark_value;
                
                // 使用RGB666格式设置窗口
                ili9488_set_window(x, y, x + grid_size - 1, y + grid_size - 1);
                
                // 填充方格
                uint8_t data[3] = {value, value, value};
                for (uint16_t i = 0; i < grid_size * grid_size; i++) {
                    ili9488_write_data_buffer(data, 3);
                }
            }
        }
        
        // 打印进度
        if (step % 10 == 0) {
            printf("黑白亮度: %d/255 (%.1f%%)\n", brightness, (float)step * 100.0f / brightness_steps);
        }
        
        // 控制时间
        current_time = to_ms_since_boot(get_absolute_time()) - start_time;
        uint32_t target_time = (step * bw_duration_ms) / brightness_steps;
        if (current_time < target_time) {
            sleep_ms(target_time - current_time);
        }
        
        // 检查是否超时
        if (current_time >= bw_duration_ms) {
            break;
        }
    }
    
    // 阶段2: RGB颜色变化
    printf("RGB颜色方格变化...\n");
    const uint8_t color_steps = 100; // 颜色变化次数
    
    for (uint8_t step = 0; step < color_steps; step++) {
        // 亮度保持最大
        ili9488_set_backlight_brightness(255);
        
        // 根据step计算RGB颜色强度 (产生颜色循环)
        float phase = (float)step / color_steps * 3.0f;
        
        // 生成RGB666格式的颜色分量（0-63范围）
        uint8_t r_intensity = (uint8_t)(31.5f + 31.5f * sin(phase * 3.14159f));
        uint8_t g_intensity = (uint8_t)(31.5f + 31.5f * sin((phase + 0.33f) * 3.14159f));
        uint8_t b_intensity = (uint8_t)(31.5f + 31.5f * sin((phase + 0.66f) * 3.14159f));
        
        // 绘制RGB方格
        for (uint16_t row = 0; row < rows; row++) {
            for (uint16_t col = 0; col < cols; col++) {
                uint16_t x = col * grid_size;
                uint16_t y = row * grid_size;
                
                // 设置窗口
                ili9488_set_window(x, y, x + grid_size - 1, y + grid_size - 1);
                
                uint16_t pattern = (row + col) % 3;
                uint8_t r = (pattern == 0) ? r_intensity : 0;
                uint8_t g = (pattern == 1) ? g_intensity : 0;
                uint8_t b = (pattern == 2) ? b_intensity : 0;
                
                // 填充方格
                uint8_t data[3] = {r, g, b};
                for (uint16_t i = 0; i < grid_size * grid_size; i++) {
                    ili9488_write_data_buffer(data, 3);
                }
            }
        }
        
        // 打印进度
        if (step % 10 == 0) {
            printf("RGB颜色变化: %.1f%%\n", (float)step * 100.0f / color_steps);
        }
        
        // 控制时间
        current_time = to_ms_since_boot(get_absolute_time()) - start_time - bw_duration_ms;
        uint32_t target_time = (step * rgb_duration_ms) / color_steps;
        if (current_time < target_time) {
            sleep_ms(target_time - current_time);
        }
        
        // 检查是否超时
        if (to_ms_since_boot(get_absolute_time()) - start_time >= total_duration_ms) {
            break;
        }
    }
    
    printf("亮度变化的黑白方格和RGB颜色演示完成\n");
}

int main() {
    // 初始化标准库
    stdio_init_all();
    sleep_ms(3000);  // 等待串口初始化
    printf("\n\n\n开始 ILI9488 LCD 演示程序...\n");
    
    // 配置LCD
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
        .rotation = 0,  // 0度旋转
    };
    
    // initialize the LCD
    if (!ili9488_init(&config)) {
        printf("错误: LCD初始化失败\n");
        return -1;
    }

    // // 运行颜色测试
    // demo_color_test();
    
    // 运行静态文字演示
    demo_static_text();
    
    // 运行动态颜色动画演示
    //demo_color_animation();
    
    // 运行新添加的演示功能
    // demo_gradient_transition();       // 新增: 渐变色过渡
    // demo_brightness_checkboard();     // 新增: 亮度变化的方格



    printf("演示完成。\n");
    
    // 保持显示最后一帧
    while (true) {
        sleep_ms(1000);
    }
    
    return 0;
} 