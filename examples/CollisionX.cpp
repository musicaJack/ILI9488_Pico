#include <stdio.h>
#include <cstdlib>
#include <random>
#include "pico/stdlib.h"
#include "joystick.hpp"
#include "joystick/joystick_config.hpp"
#include "ili9488_driver.hpp"
#include "ili9488_colors.hpp"
#include "ili9488_font.hpp"

// 横屏模式 - ILI9488分辨率调整
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

// 方块和游戏相关常量 - 根据大屏调整
#define BLOCK_SIZE 30
#define MOVE_STEP 8

// 红线常量
#define LINE_WIDTH 8
#define TOP_LINE_Y 25
#define BOTTOM_LINE_Y (SCREEN_HEIGHT - 25 - LINE_WIDTH)

// 游戏常量
#define GAME_TIME 20
#define MAX_STAMPS 50
#define MAX_DOTS 10

// 颜色定义 - 使用ILI9488颜色
#define TEXT_COLOR ili9488_colors::rgb565::WHITE
#define BG_COLOR ili9488_colors::rgb565::BLACK
#define BLOCK_COLOR ili9488_colors::rgb565::BLUE
#define STAMP_COLOR ili9488_colors::rgb565::YELLOW
#define IRON_BLOCK_COLOR ili9488_colors::rgb565::CYAN
#define DOT_COLOR ili9488_colors::rgb565::GREEN
#define YELLOW_DOT_COLOR ili9488_colors::rgb565::YELLOW
#define LINE_COLOR ili9488_colors::rgb565::RED

// 游戏特定常量
#define JOYSTICK_DIRECTION_RATIO 1.5

// 数据结构
struct BlockPosition {
    int16_t x;
    int16_t y;
};

struct Stamp {
    BlockPosition pos;
    bool is_iron;  // 是否为铁方块
};

struct StampPositions {
    Stamp stamps[MAX_STAMPS];
    uint8_t count;
};

struct WanderingDot {
    BlockPosition pos;
    int16_t speed_x;
    int16_t speed_y;
    bool active;
    bool is_yellow;
};

struct WanderingDots {
    WanderingDot dots[MAX_DOTS];
    uint8_t count;
};

// 绘制方块
void drawBlock(ili9488::ILI9488Driver& driver, const BlockPosition& pos) {
    driver.fillArea(pos.x, pos.y, pos.x + BLOCK_SIZE - 1, pos.y + BLOCK_SIZE - 1, 
                   ili9488_colors::rgb565_to_rgb888(BLOCK_COLOR));
}

// 清除方块
void clearBlock(ili9488::ILI9488Driver& driver, const BlockPosition& pos) {
    driver.fillArea(pos.x, pos.y, pos.x + BLOCK_SIZE - 1, pos.y + BLOCK_SIZE - 1, 
                   ili9488_colors::rgb565_to_rgb888(BG_COLOR));
}

// 绘制盖章方块
void drawStamp(ili9488::ILI9488Driver& driver, const Stamp& stamp) {
    uint16_t color = stamp.is_iron ? IRON_BLOCK_COLOR : STAMP_COLOR;
    driver.fillArea(stamp.pos.x, stamp.pos.y, stamp.pos.x + BLOCK_SIZE - 1, stamp.pos.y + BLOCK_SIZE - 1, 
                   ili9488_colors::rgb565_to_rgb888(color));
}

// 绘制所有盖章
void drawAllStamps(ili9488::ILI9488Driver& driver, const StampPositions& stamps) {
    for (uint8_t i = 0; i < stamps.count; i++) {
        drawStamp(driver, stamps.stamps[i]);
    }
}

// 绘制圆点 - 使用多个小矩形模拟圆形
void drawDot(ili9488::ILI9488Driver& driver, const BlockPosition& pos, bool is_yellow = false) {
    uint16_t color = is_yellow ? YELLOW_DOT_COLOR : DOT_COLOR;
    uint32_t color888 = ili9488_colors::rgb565_to_rgb888(color);
    
    // 简单的圆形近似 - 使用矩形
    int16_t center_x = pos.x + BLOCK_SIZE/2;
    int16_t center_y = pos.y + BLOCK_SIZE/2;
    int16_t radius = BLOCK_SIZE/3;
    
    driver.fillArea(center_x - radius, center_y - radius, 
                   center_x + radius, center_y + radius, color888);
}

// 清除圆点
void clearDot(ili9488::ILI9488Driver& driver, const BlockPosition& pos) {
    uint32_t bg_color888 = ili9488_colors::rgb565_to_rgb888(BG_COLOR);
    
    int16_t center_x = pos.x + BLOCK_SIZE/2;
    int16_t center_y = pos.y + BLOCK_SIZE/2;
    int16_t radius = BLOCK_SIZE/3 + 1;
    
    driver.fillArea(center_x - radius, center_y - radius, 
                   center_x + radius, center_y + radius, bg_color888);
}

// 绘制所有圆点
void drawAllDots(ili9488::ILI9488Driver& driver, const WanderingDots& dots) {
    for (uint8_t i = 0; i < dots.count; i++) {
        if (dots.dots[i].active) {
            drawDot(driver, dots.dots[i].pos, dots.dots[i].is_yellow);
        }
    }
}

// 清除所有圆点
void clearAllDots(ili9488::ILI9488Driver& driver, const WanderingDots& dots) {
    for (uint8_t i = 0; i < dots.count; i++) {
        if (dots.dots[i].active) {
            clearDot(driver, dots.dots[i].pos);
        }
    }
}

// 绘制红线
void drawLines(ili9488::ILI9488Driver& driver) {
    uint32_t line_color888 = ili9488_colors::rgb565_to_rgb888(LINE_COLOR);
    driver.fillArea(0, TOP_LINE_Y, SCREEN_WIDTH - 1, TOP_LINE_Y + LINE_WIDTH - 1, line_color888);
    driver.fillArea(0, BOTTOM_LINE_Y, SCREEN_WIDTH - 1, BOTTOM_LINE_Y + LINE_WIDTH - 1, line_color888);
}

// 绘制倒计时
void drawCountdown(ili9488::ILI9488Driver& driver, int remaining_seconds) {
    char time_str[10];
    snprintf(time_str, sizeof(time_str), "Time: %02d", remaining_seconds);
    
    // 清除倒计时区域
    driver.fillArea(SCREEN_WIDTH - 100, 5, SCREEN_WIDTH - 1, 25, 
                   ili9488_colors::rgb565_to_rgb888(BG_COLOR));
    
    // 绘制倒计时文字
    driver.drawString(SCREEN_WIDTH - 95, 10, time_str, 
                     ili9488_colors::rgb565_to_rgb888(TEXT_COLOR), 
                     ili9488_colors::rgb565_to_rgb888(BG_COLOR));
}

// 摇杆方向判断
int determine_joystick_direction(int16_t x, int16_t y) {
    int16_t abs_x = abs(x);
    int16_t abs_y = abs(y);
    
    if (abs_x < JOYSTICK_THRESHOLD && abs_y < JOYSTICK_THRESHOLD) {
        return 0;
    }
    
    if (abs_x < JOYSTICK_THRESHOLD * 0.8 && abs_y < JOYSTICK_THRESHOLD * 0.8) {
        return 0;
    }
    
    if (abs_y > abs_x * JOYSTICK_DIRECTION_RATIO) {
        return (y < 0) ? 1 : 2;  // 1=上, 2=下
    }
    
    if (abs_x > abs_y * JOYSTICK_DIRECTION_RATIO) {
        return (x < 0) ? 3 : 4;  // 3=左, 4=右
    }
    
    return 0;
}

// 检查红线碰撞
bool checkLineCollision(const BlockPosition& pos) {
    return (pos.y <= TOP_LINE_Y + LINE_WIDTH) || 
           (pos.y + BLOCK_SIZE >= BOTTOM_LINE_Y);
}

// 检查位置是否被占用
bool isPositionOccupied(const BlockPosition& pos, const StampPositions& stamps) {
    for (uint8_t i = 0; i < stamps.count; i++) {
        if (abs(pos.x - stamps.stamps[i].pos.x) < BLOCK_SIZE &&
            abs(pos.y - stamps.stamps[i].pos.y) < BLOCK_SIZE) {
            return true;
        }
    }
    return false;
}

// 检查位置是否在有效区域
bool isPositionInValidArea(const BlockPosition& pos) {
    return (pos.y > TOP_LINE_Y + LINE_WIDTH + 5) && 
           (pos.y + BLOCK_SIZE < BOTTOM_LINE_Y - 5);
}

// 添加盖章
void addStamp(StampPositions& stamps, const BlockPosition& pos, bool is_iron = false) {
    if (stamps.count < MAX_STAMPS) {
        stamps.stamps[stamps.count].pos = pos;
        stamps.stamps[stamps.count].is_iron = is_iron;
        stamps.count++;
    }
}

// 检查与盖章碰撞
bool checkStampCollision(const BlockPosition& pos, const StampPositions& stamps) {
    for (uint8_t i = 0; i < stamps.count; i++) {
        if (abs(pos.x - stamps.stamps[i].pos.x) < BLOCK_SIZE &&
            abs(pos.y - stamps.stamps[i].pos.y) < BLOCK_SIZE) {
            return true;
        }
    }
    return false;
}

// 更新圆点位置
void updateDots(WanderingDots& dots, const StampPositions& stamps) {
    for (uint8_t i = 0; i < dots.count; i++) {
        if (!dots.dots[i].active) continue;
        
        // 备份当前位置
        BlockPosition old_pos = dots.dots[i].pos;
        
        // 尝试更新位置
        dots.dots[i].pos.x += dots.dots[i].speed_x;
        dots.dots[i].pos.y += dots.dots[i].speed_y;
        
        // 检查边界碰撞
        bool hit_boundary = false;
        if (dots.dots[i].pos.x <= 0 || dots.dots[i].pos.x >= SCREEN_WIDTH - BLOCK_SIZE) {
            dots.dots[i].speed_x = -dots.dots[i].speed_x;
            hit_boundary = true;
        }
        if (dots.dots[i].pos.y <= 0 || dots.dots[i].pos.y >= SCREEN_HEIGHT - BLOCK_SIZE) {
            dots.dots[i].speed_y = -dots.dots[i].speed_y;
            hit_boundary = true;
        }
        
        // 检查盖章碰撞
        if (checkStampCollision(dots.dots[i].pos, stamps)) {
            // 反弹
            dots.dots[i].speed_x = -dots.dots[i].speed_x;
            dots.dots[i].speed_y = -dots.dots[i].speed_y;
            dots.dots[i].pos = old_pos;  // 恢复到碰撞前位置
        }
        
        // 边界修正
        if (hit_boundary) {
            if (dots.dots[i].pos.x < 0) dots.dots[i].pos.x = 0;
            if (dots.dots[i].pos.x >= SCREEN_WIDTH - BLOCK_SIZE) 
                dots.dots[i].pos.x = SCREEN_WIDTH - BLOCK_SIZE - 1;
            if (dots.dots[i].pos.y < 0) dots.dots[i].pos.y = 0;
            if (dots.dots[i].pos.y >= SCREEN_HEIGHT - BLOCK_SIZE) 
                dots.dots[i].pos.y = SCREEN_HEIGHT - BLOCK_SIZE - 1;
        }
    }
}

// 添加游走圆点
void addWanderingDot(WanderingDots& dots, bool is_yellow = false) {
    if (dots.count >= MAX_DOTS) return;
    
    // 在中间区域生成
    BlockPosition start_pos = {
        static_cast<int16_t>(rand() % (SCREEN_WIDTH - BLOCK_SIZE - 100) + 50),
        static_cast<int16_t>(rand() % (BOTTOM_LINE_Y - TOP_LINE_Y - 100) + TOP_LINE_Y + 50)
    };
    
    // 随机速度 - 适配大屏幕
    int16_t speed_x = (rand() % 8) - 4;  // -4 到 4
    int16_t speed_y = (rand() % 8) - 4;
    
    // 确保速度不为0
    if (speed_x == 0) speed_x = 1;
    if (speed_y == 0) speed_y = 1;
    
    dots.dots[dots.count].pos = start_pos;
    dots.dots[dots.count].speed_x = speed_x;
    dots.dots[dots.count].speed_y = speed_y;
    dots.dots[dots.count].active = true;
    dots.dots[dots.count].is_yellow = is_yellow;
    dots.count++;
}

int main() {
    stdio_init_all();
    printf("CollisionX Game for ILI9488 - Landscape Mode\n");
    
    // 初始化随机数
    srand(to_ms_since_boot(get_absolute_time()));
    
    // 初始化ILI9488显示屏
    ili9488::ILI9488Driver lcd_driver(
        spi0, 20, 15, 17, 18, 19, 10, 40000000
    );
    
    if (!lcd_driver.initialize()) {
        printf("LCD initialization failed!\n");
        return -1;
    }
    
    // 设置为横屏模式
    lcd_driver.setRotation(ili9488::Rotation::Landscape_90);
    
    // 初始化摇杆
    Joystick joystick;
    if (!joystick.begin(JOYSTICK_I2C_PORT, JOYSTICK_I2C_ADDR, 
                       JOYSTICK_I2C_SDA_PIN, JOYSTICK_I2C_SCL_PIN, 
                       JOYSTICK_I2C_SPEED)) {
        printf("Joystick initialization failed!\n");
        return -1;
    }
    
    printf("Initialization successful!\n");
    joystick.set_rgb_color(JOYSTICK_LED_GREEN);
    sleep_ms(1000);
    joystick.set_rgb_color(JOYSTICK_LED_OFF);
    
    // 显示启动画面
    lcd_driver.fillScreen(BG_COLOR);
    lcd_driver.drawString(150, 160, "Press MID BTN to start", 
                         ili9488_colors::rgb565_to_rgb888(TEXT_COLOR), 
                         ili9488_colors::rgb565_to_rgb888(BG_COLOR));
    
    // 等待开始
    bool started = false;
    while (!started) {
        if (joystick.get_button_value() == 0) {
            started = true;
            lcd_driver.fillScreen(BG_COLOR);
            drawLines(lcd_driver);
            sleep_ms(200);
        }
        sleep_ms(JOYSTICK_LOOP_DELAY_MS);
    }
    
    // 初始化游戏状态
    BlockPosition block_pos = {
        (SCREEN_WIDTH - BLOCK_SIZE) / 2,
        (SCREEN_HEIGHT - BLOCK_SIZE) / 2
    };
    
    drawBlock(lcd_driver, block_pos);
    
    // 游戏变量
    static int previous_raw_direction = 0;
    static uint8_t stable_count = 0;
    static StampPositions stamps = {{}, 0};
    WanderingDots wandering_dots = {{}, 0};
    
    bool game_paused = false;
    bool game_started = false;
    uint32_t game_start_time = 0;
    int remaining_seconds = GAME_TIME;
    
    // LED状态变量
    static bool is_active = false;
    static bool last_mid_pressed = false;
    static absolute_time_t last_red_time = {0};
    
    // 主游戏循环
    while (true) {
        // 按钮处理
        static uint32_t button_press_start_time = 0;
        static bool button_pressed = false;
        static bool long_press_triggered = false;
        
        // 检查MID按钮状态
        bool mid_pressed = (joystick.get_button_value() == 0);
        
        // 检测mid键按下的瞬间 - 红灯逻辑
        if (mid_pressed && !last_mid_pressed) {
            joystick.set_rgb_color(JOYSTICK_LED_RED);
            last_red_time = get_absolute_time();
        }
        
        // 50ms后自动关闭红灯
        if (!is_nil_time(last_red_time) && absolute_time_diff_us(last_red_time, get_absolute_time()) > 50000) {
            joystick.set_rgb_color(JOYSTICK_LED_OFF);
            last_red_time = {0};
        }
        
        if (mid_pressed) {
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            
            if (!button_pressed) {
                button_pressed = true;
                button_press_start_time = current_time;
                long_press_triggered = false;
                
                if (game_paused) {
                    game_paused = false;
                    game_started = false;
                    remaining_seconds = GAME_TIME;
                    stamps.count = 0;
                    wandering_dots.count = 0;
                    lcd_driver.fillScreen(BG_COLOR);
                    drawLines(lcd_driver);
                    drawBlock(lcd_driver, block_pos);
                    continue;
                }
                
                // 短按：放置或升级方块
                if (isPositionInValidArea(block_pos)) {
                    bool found_existing = false;
                    
                    printf("Placing block at position: (%d, %d)\n", block_pos.x, block_pos.y);
                    
                    // 检查是否已有方块
                    for (uint8_t i = 0; i < stamps.count; i++) {
                        if (abs(block_pos.x - stamps.stamps[i].pos.x) < BLOCK_SIZE &&
                            abs(block_pos.y - stamps.stamps[i].pos.y) < BLOCK_SIZE) {
                            // 升级为铁方块
                            if (!stamps.stamps[i].is_iron) {
                                stamps.stamps[i].is_iron = true;
                                drawStamp(lcd_driver, stamps.stamps[i]);
                                printf("Upgraded block to iron at: (%d, %d)\n", stamps.stamps[i].pos.x, stamps.stamps[i].pos.y);
                            }
                            found_existing = true;
                            break;
                        }
                    }
                    
                    if (!found_existing) {
                        // 添加新方块
                        addStamp(stamps, block_pos, false);
                        drawStamp(lcd_driver, stamps.stamps[stamps.count - 1]);
                        printf("Added new stamp block at: (%d, %d), total stamps: %d\n", 
                               block_pos.x, block_pos.y, stamps.count);
                    }
                }
            }
            
            // 长按检测（3秒）
            if (button_pressed && !long_press_triggered && 
                (current_time - button_press_start_time) >= 3000) {
                long_press_triggered = true;
                
                if (!game_started) {
                    // 开始游戏，释放绿球
                    game_started = true;
                    game_start_time = current_time;
                    addWanderingDot(wandering_dots, false);  // 绿球
                    
                    // 30%概率生成小黄球
                    if (rand() % 100 < 30) {
                        addWanderingDot(wandering_dots, true);  // 黄球
                    }
                }
            }
        } else {
            button_pressed = false;
            long_press_triggered = false;
        }
        
        last_mid_pressed = mid_pressed;
        
        if (game_paused) {
            sleep_ms(JOYSTICK_LOOP_DELAY_MS);
            continue;
        }
        
        // 倒计时更新
        if (game_started) {
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            int elapsed_seconds = (current_time - game_start_time) / 1000;
            remaining_seconds = GAME_TIME - elapsed_seconds;
            
            if (remaining_seconds <= 0) {
                game_paused = true;
                lcd_driver.drawString(200, 160, "You Win!", 
                                     ili9488_colors::rgb565_to_rgb888(TEXT_COLOR), 
                                     ili9488_colors::rgb565_to_rgb888(BG_COLOR));
                sleep_ms(5000);
                game_paused = false;
                game_started = false;
                remaining_seconds = GAME_TIME;
                stamps.count = 0;
                wandering_dots.count = 0;
                lcd_driver.fillScreen(BG_COLOR);
                drawLines(lcd_driver);
                continue;
            }
            
            drawCountdown(lcd_driver, remaining_seconds);
        }
        
        // 摇杆控制
        uint16_t adc_x = 0, adc_y = 0;
        joystick.get_joy_adc_16bits_value_xy(&adc_x, &adc_y);
        int16_t offset_x = joystick.get_joy_adc_12bits_offset_value_x();
        int16_t offset_y = joystick.get_joy_adc_12bits_offset_value_y();
        int raw_direction = determine_joystick_direction(offset_x, offset_y);
        
        // 方向稳定性检查
        if (raw_direction == previous_raw_direction) {
            stable_count++;
        } else {
            stable_count = 0;
            previous_raw_direction = raw_direction;
        }
        
        // 移动方块
        if (stable_count >= 3 && raw_direction != 0) {
            clearBlock(lcd_driver, block_pos);
            
            BlockPosition new_pos = block_pos;
            switch (raw_direction) {
                case 1: new_pos.y -= MOVE_STEP; break; // 上
                case 2: new_pos.y += MOVE_STEP; break; // 下
                case 3: new_pos.x -= MOVE_STEP; break; // 左
                case 4: new_pos.x += MOVE_STEP; break; // 右
            }
            
            // 边界检查
            if (new_pos.x >= 0 && new_pos.x <= SCREEN_WIDTH - BLOCK_SIZE &&
                new_pos.y >= 0 && new_pos.y <= SCREEN_HEIGHT - BLOCK_SIZE) {
                block_pos = new_pos;
            }
            
            drawBlock(lcd_driver, block_pos);
            stable_count = 0;
        }
        
        // 摇杆LED控制逻辑（蓝灯）- 只在没有按钮按下且红灯不亮时控制
        if (!mid_pressed && last_red_time == (absolute_time_t){0}) {
            if (raw_direction > 0 && !is_active) {
                is_active = true;
                joystick.set_rgb_color(JOYSTICK_LED_BLUE);
            } else if (raw_direction == 0 && is_active) {
                is_active = false;
                joystick.set_rgb_color(JOYSTICK_LED_OFF);
            }
        }
        
        // 更新圆点
        if (game_started && wandering_dots.count > 0) {
            clearAllDots(lcd_driver, wandering_dots);
            updateDots(wandering_dots, stamps);
            drawAllDots(lcd_driver, wandering_dots);
            
            // 检查游戏失败
            for (uint8_t i = 0; i < wandering_dots.count; i++) {
                if (wandering_dots.dots[i].active && 
                    checkLineCollision(wandering_dots.dots[i].pos)) {
                    game_paused = true;
                    lcd_driver.drawString(190, 160, "You Lost!", 
                                         ili9488_colors::rgb565_to_rgb888(TEXT_COLOR), 
                                         ili9488_colors::rgb565_to_rgb888(BG_COLOR));
                    sleep_ms(5000);
                    game_paused = false;
                    game_started = false;
                    remaining_seconds = GAME_TIME;
                    stamps.count = 0;
                    wandering_dots.count = 0;
                    lcd_driver.fillScreen(BG_COLOR);
                    drawLines(lcd_driver);
                    break;
                }
            }
        }
        
        sleep_ms(JOYSTICK_LOOP_DELAY_MS);
    }
} 