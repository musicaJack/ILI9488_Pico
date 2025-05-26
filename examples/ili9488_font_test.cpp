/**
 * @file ili9488_font_test.cpp
 * @brief Font System Test for ILI9488 (ST73xx compatible)
 */

#include <cstdio>
#include <cstdint>

#include "pico/stdlib.h"
#include "hardware/spi.h"

// Modern C++ ILI9488 driver includes
#include "ili9488_driver.hpp"
#include "ili9488_colors.hpp"
#include "ili9488_font.hpp"

// Hardware pin definitions
constexpr uint8_t PIN_DC   = 20;
constexpr uint8_t PIN_RST  = 15;
constexpr uint8_t PIN_CS   = 17;
constexpr uint8_t PIN_SCK  = 18;
constexpr uint8_t PIN_MOSI = 19;
constexpr uint8_t PIN_BL   = 10;

using namespace ili9488;
using namespace ili9488_colors;

// 字体测试函数 - 打印字符位图
void printCharBitmap(char c) {
    printf("Character '%c' (0x%02X):\n", c, (unsigned char)c);
    const uint8_t* data = font::get_char_data(c);
    
    for (int i = 0; i < font::FONT_HEIGHT; ++i) {
        for (int b = 7; b >= 0; --b) {
            putchar((data[i] & (1 << b)) ? '#' : '.');
        }
        putchar('\n');
    }
    printf("\n");
}

// 字体系统API测试
void testFontSystem() {
    printf("=== Font System API Test ===\n");
    printf("Font dimensions: %d x %d\n", font::FONT_WIDTH, font::FONT_HEIGHT);
    printf("Font data size: %d bytes\n", font::FONT_SIZE);
    
    // 测试一些基本字符
    printCharBitmap('A');
    printCharBitmap('B');
    printCharBitmap('1');
    printCharBitmap('!');
    
    printf("Font system test completed!\n\n");
}

// 显示器字体测试
void testDisplayFont(ILI9488Driver& driver) {
    printf("=== Display Font Test ===\n");
    
    // 清屏
    driver.fillScreen(rgb565::BLACK);
    
    // 绘制测试文本
    uint16_t y = 10;
    
    // 测试基本文本
    driver.drawString(10, y, "Hello, ILI9488!", rgb565::GREEN, rgb565::BLACK);
    y += font::FONT_HEIGHT + 5;
    
    // 测试数字
    driver.drawString(10, y, "Numbers: 0123456789", rgb565::RED, rgb565::BLACK);
    y += font::FONT_HEIGHT + 5;
    
    // 测试符号
    driver.drawString(10, y, "Symbols: !@#$%^&*()", rgb565::BLUE, rgb565::BLACK);
    y += font::FONT_HEIGHT + 5;
    
    // 测试大小写字母
    driver.drawString(10, y, "ABCDEFGHIJKLMNOPQRST", rgb565::YELLOW, rgb565::BLACK);
    y += font::FONT_HEIGHT + 5;
    
    driver.drawString(10, y, "abcdefghijklmnopqrst", rgb565::MAGENTA, rgb565::BLACK);
    y += font::FONT_HEIGHT + 5;
    
    // 测试长文本换行
    const char* long_text = "This is a long text to test line wrapping and font rendering performance.";
    uint16_t x = 10;
    y += 10;
    
    for (const char* p = long_text; *p; ++p) {
        if (x + font::FONT_WIDTH > driver.getWidth() - 10) {
            x = 10;
            y += font::FONT_HEIGHT + 2;
        }
        
        if (*p == ' ') {
            x += font::FONT_WIDTH;
        } else {
            driver.drawChar(x, y, *p, rgb565::BLACK, rgb565::WHITE);
            x += font::FONT_WIDTH;
        }
    }
    
    printf("Display font test completed!\n");
}

int main() {
    stdio_init_all();
    printf("=== ILI9488 Font System Test (ST73xx Compatible) ===\n");
    
    // 1. 测试字体系统API
    testFontSystem();
    
    // 2. 初始化显示器
    ILI9488Driver driver(spi0, PIN_DC, PIN_RST, PIN_CS, PIN_SCK, PIN_MOSI, PIN_BL);
    
    if (!driver.initialize()) {
        printf("Failed to initialize display!\n");
        return -1;
    }
    
    driver.setBacklight(true);
    printf("Display initialized successfully!\n");
    
    // 3. 测试显示器上的字体渲染
    testDisplayFont(driver);
    
    printf("\n=== Font system tests completed! ===\n");
    printf("Font system is now fully compatible with ST73xx design.\n");
    printf("Key features:\n");
    printf("- Simple 'font' namespace (like ST73xx)\n");
    printf("- get_char_data(char c) API (like ST73xx)\n");
    printf("- 8x16 pixel font data\n");
    printf("- 256 character support (ASCII 0-255)\n");
    printf("- Row-based bitmap layout\n");
    
    while (true) {
        sleep_ms(1000);
    }
    
    return 0;
} 