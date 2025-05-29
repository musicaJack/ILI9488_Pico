#include <stdio.h>
#include <cstdlib>
#include <random>
#include "pico/stdlib.h"
#include "joystick.hpp"
#include "joystick/joystick_config.hpp"
#include "ili9488_driver.hpp"
#include "ili9488_colors.hpp"
#include "ili9488_font.hpp"

// 竖屏模式 - ILI9488分辨率调整
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

// 游戏网格设置
#define GRID_SIZE 16
#define GRID_WIDTH (SCREEN_WIDTH / GRID_SIZE)
#define GRID_HEIGHT (SCREEN_HEIGHT / GRID_SIZE)

// 游戏常量
#define MAX_SNAKE_LENGTH 200
#define INITIAL_SNAKE_LENGTH 3
#define GAME_SPEED_MS 200

// 颜色定义 - 统一使用RGB666格式（ILI9488原生格式，无需转换）
#define TEXT_COLOR ili9488_colors::rgb666::WHITE
#define BG_COLOR ili9488_colors::rgb666::BLACK
#define SNAKE_HEAD_COLOR ili9488_colors::rgb666::BRIGHT_MAGENTA
#define SNAKE_BODY_COLOR ili9488_colors::rgb666::NEON_GREEN
#define FOOD_COLOR ili9488_colors::rgb666::GREENYELLOW
#define BORDER_COLOR ili9488_colors::rgb666::BLUE

// 摇杆方向常量
#define JOYSTICK_DIRECTION_RATIO 1.5

// 方向枚举
enum Direction {
    DIR_NONE = 0,
    DIR_UP = 1,
    DIR_DOWN = 2,
    DIR_LEFT = 3,
    DIR_RIGHT = 4
};

// 位置结构
struct Position {
    int16_t x;
    int16_t y;
};

// 蛇的结构
struct Snake {
    Position segments[MAX_SNAKE_LENGTH];
    uint16_t length;
    Direction direction;
    Direction next_direction;
};

// 游戏状态结构
struct GameState {
    Snake snake;
    Position food;
    uint16_t score;
    bool game_over;
    bool game_paused;
    bool game_started;
    bool waiting_to_restart;  // 倒计时结束后等待用户按键重新开始
    uint32_t game_over_time;  // 游戏结束时间（毫秒）
};

// 绘制网格单元
void drawGridCell(ili9488::ILI9488Driver& driver, int16_t grid_x, int16_t grid_y, uint32_t color666) {
    int16_t pixel_x = grid_x * GRID_SIZE;
    int16_t pixel_y = grid_y * GRID_SIZE;
    driver.fillAreaRGB666(pixel_x, pixel_y, pixel_x + GRID_SIZE - 1, pixel_y + GRID_SIZE - 1, color666);
}

// 清除网格单元
void clearGridCell(ili9488::ILI9488Driver& driver, int16_t grid_x, int16_t grid_y) {
    drawGridCell(driver, grid_x, grid_y, BG_COLOR);
}

// 绘制边框
void drawBorder(ili9488::ILI9488Driver& driver) {
    // 顶部边框
    driver.fillAreaRGB666(0, 0, SCREEN_WIDTH - 1, GRID_SIZE - 1, BORDER_COLOR);
    // 底部边框
    driver.fillAreaRGB666(0, SCREEN_HEIGHT - GRID_SIZE, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BORDER_COLOR);
    // 左边框
    driver.fillAreaRGB666(0, 0, GRID_SIZE - 1, SCREEN_HEIGHT - 1, BORDER_COLOR);
    // 右边框
    driver.fillAreaRGB666(SCREEN_WIDTH - GRID_SIZE, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BORDER_COLOR);
}

// 绘制蛇
void drawSnake(ili9488::ILI9488Driver& driver, const Snake& snake) {
    // 绘制蛇头
    if (snake.length > 0) {
        drawGridCell(driver, snake.segments[0].x, snake.segments[0].y, SNAKE_HEAD_COLOR);
    }
    
    // 绘制蛇身
    for (uint16_t i = 1; i < snake.length; i++) {
        drawGridCell(driver, snake.segments[i].x, snake.segments[i].y, SNAKE_BODY_COLOR);
    }
}

// 清除蛇尾
void clearSnakeTail(ili9488::ILI9488Driver& driver, const Position& tail_pos) {
    clearGridCell(driver, tail_pos.x, tail_pos.y);
}

// 绘制食物
void drawFood(ili9488::ILI9488Driver& driver, const Position& food_pos) {
    printf("Drawing food at grid (%d, %d), pixel (%d, %d)\n", 
           food_pos.x, food_pos.y, 
           food_pos.x * GRID_SIZE, food_pos.y * GRID_SIZE);
    drawGridCell(driver, food_pos.x, food_pos.y, FOOD_COLOR);
}

// 生成随机食物位置
void generateFood(GameState& game_state) {
    bool valid_position = false;
    
    while (!valid_position) {
        // 在游戏区域内生成（避开边框）
        game_state.food.x = (rand() % (GRID_WIDTH - 2)) + 1;
        game_state.food.y = (rand() % (GRID_HEIGHT - 2)) + 1;
        
        printf("Generated food at grid position: (%d, %d), Grid size: %dx%d\n", 
               game_state.food.x, game_state.food.y, GRID_WIDTH, GRID_HEIGHT);
        
        valid_position = true;
        
        // 检查是否与蛇身重叠
        for (uint16_t i = 0; i < game_state.snake.length; i++) {
            if (game_state.snake.segments[i].x == game_state.food.x &&
                game_state.snake.segments[i].y == game_state.food.y) {
                valid_position = false;
                printf("Food position conflicts with snake segment %d\n", i);
                break;
            }
        }
    }
    
    printf("Final food position: (%d, %d)\n", game_state.food.x, game_state.food.y);
}

// 初始化游戏状态
void initializeGame(GameState& game_state) {
    // 初始化蛇
    game_state.snake.length = INITIAL_SNAKE_LENGTH;
    game_state.snake.direction = DIR_RIGHT;
    game_state.snake.next_direction = DIR_RIGHT;
    
    // 蛇的初始位置（屏幕中央）
    int16_t start_x = GRID_WIDTH / 2;
    int16_t start_y = GRID_HEIGHT / 2;
    
    for (uint16_t i = 0; i < INITIAL_SNAKE_LENGTH; i++) {
        game_state.snake.segments[i].x = start_x - i;
        game_state.snake.segments[i].y = start_y;
    }
    
    // 初始化游戏状态
    game_state.score = 0;
    game_state.game_over = false;
    game_state.game_paused = false;
    game_state.game_started = false;
    game_state.waiting_to_restart = false;
    game_state.game_over_time = 0;
    
    // 生成第一个食物
    generateFood(game_state);
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

// 检查方向是否相反
bool isOppositeDirection(Direction current, Direction new_dir) {
    return (current == DIR_UP && new_dir == DIR_DOWN) ||
           (current == DIR_DOWN && new_dir == DIR_UP) ||
           (current == DIR_LEFT && new_dir == DIR_RIGHT) ||
           (current == DIR_RIGHT && new_dir == DIR_LEFT);
}

// 移动蛇
bool moveSnake(GameState& game_state) {
    // 更新方向（如果不是相反方向）
    if (game_state.snake.next_direction != DIR_NONE && 
        !isOppositeDirection(game_state.snake.direction, game_state.snake.next_direction)) {
        game_state.snake.direction = game_state.snake.next_direction;
    }
    
    // 计算新的头部位置
    Position new_head = game_state.snake.segments[0];
    
    switch (game_state.snake.direction) {
        case DIR_UP:
            new_head.y--;
            break;
        case DIR_DOWN:
            new_head.y++;
            break;
        case DIR_LEFT:
            new_head.x--;
            break;
        case DIR_RIGHT:
            new_head.x++;
            break;
        default:
            return true; // 没有方向，不移动
    }
    
    // 检查边界碰撞
    if (new_head.x <= 0 || new_head.x >= GRID_WIDTH - 1 ||
        new_head.y <= 0 || new_head.y >= GRID_HEIGHT - 1) {
        return false; // 游戏结束
    }
    
    // 检查自身碰撞
    for (uint16_t i = 0; i < game_state.snake.length; i++) {
        if (game_state.snake.segments[i].x == new_head.x &&
            game_state.snake.segments[i].y == new_head.y) {
            return false; // 游戏结束
        }
    }
    
    // 检查是否吃到食物
    bool ate_food = (new_head.x == game_state.food.x && new_head.y == game_state.food.y);
    
    if (ate_food) {
        // 增长蛇身
        if (game_state.snake.length < MAX_SNAKE_LENGTH) {
            game_state.snake.length++;
        }
        game_state.score += 10;
        generateFood(game_state);
    }
    
    // 移动蛇身（从尾部开始）
    for (uint16_t i = game_state.snake.length - 1; i > 0; i--) {
        game_state.snake.segments[i] = game_state.snake.segments[i - 1];
    }
    
    // 设置新的头部位置
    game_state.snake.segments[0] = new_head;
    
    return true;
}

// 绘制分数
void drawScore(ili9488::ILI9488Driver& driver, uint16_t score) {
    char score_str[20];
    snprintf(score_str, sizeof(score_str), "Score: %d", score);
    
    // 在顶部边框区域内显示分数，使用边框颜色作为背景
    // 顶部边框高度是GRID_SIZE(16像素)，文字居中显示
    int16_t text_x = 5;  // 左边距5像素
    int16_t text_y = 2;  // 距离顶部2像素，让文字在边框内居中
    
    // 清除分数区域，使用边框颜色
    driver.fillAreaRGB666(text_x, text_y, text_x + 120, text_y + 12, BORDER_COLOR);
    
    // 绘制分数文字，白色文字在蓝色边框背景上
    driver.drawString(text_x, text_y, score_str, 
                     TEXT_COLOR, 
                     BORDER_COLOR);
}

// 绘制游戏结束画面
void drawGameOver(ili9488::ILI9488Driver& driver, uint16_t final_score) {
    // 清除中央区域
    driver.fillAreaRGB666(50, 200, 270, 280, BG_COLOR);
    
    // 绘制游戏结束文字
    driver.drawString(120, 210, "Game Over!", 
                     TEXT_COLOR, 
                     BG_COLOR);
    
    char score_str[30];
    snprintf(score_str, sizeof(score_str), "Final Score: %d", final_score);
    driver.drawString(90, 230, score_str, 
                     TEXT_COLOR, 
                     BG_COLOR);
    
    driver.drawString(70, 250, "Auto restart in 5 seconds", 
                     TEXT_COLOR, 
                     BG_COLOR);
}

// 绘制暂停画面
void drawPaused(ili9488::ILI9488Driver& driver) {
    // 清除中央区域 - 扩大区域以确保完全覆盖文字
    driver.fillAreaRGB666(70, 220, 250, 270, BG_COLOR);
    
    // 绘制暂停文字
    driver.drawString(130, 230, "PAUSED", 
                     TEXT_COLOR, 
                     BG_COLOR);
    
    driver.drawString(90, 250, "Press MID to resume", 
                     TEXT_COLOR, 
                     BG_COLOR);
}

// 清除暂停文字（只清除暂停文字区域）
void clearPaused(ili9488::ILI9488Driver& driver) {
    // 只清除暂停文字区域 - 使用与drawPaused相同的区域
    driver.fillAreaRGB666(70, 220, 250, 270, BG_COLOR);
}

// 更新倒计时数字（只更新数字部分）
void updateCountdown(ili9488::ILI9488Driver& driver, uint32_t countdown_seconds) {
    char number_str[5];
    snprintf(number_str, sizeof(number_str), "%lu", (unsigned long)countdown_seconds);
    
    // 计算 "Auto restart in " 的像素宽度
    // 假设每个字符约8像素宽度，"Auto restart in " 有16个字符 = 128像素
    int16_t text_start_x = 70;  // 原始文字起始位置
    int16_t number_start_x = text_start_x + 128;  // 数字开始位置
    
    // 清除数字区域，只清除1-2位数字的空间，不影响后面的"seconds"
    // 数字最多2位，每位约8像素，给16像素就够了
    driver.fillAreaRGB666(number_start_x, 250, number_start_x + 16, 270, 
                   BG_COLOR);
    
    // 在正确位置重绘数字
    driver.drawString(number_start_x, 250, number_str, 
                     TEXT_COLOR, 
                     BG_COLOR);
}

// 绘制等待重新开始画面
void drawWaitingToRestart(ili9488::ILI9488Driver& driver, uint16_t final_score) {
    // 清除中央区域
    driver.fillAreaRGB666(50, 200, 270, 280, BG_COLOR);
    
    // 绘制等待重新开始文字
    driver.drawString(100, 210, "Game Over!", 
                     TEXT_COLOR, 
                     BG_COLOR);
    
    char score_str[30];
    snprintf(score_str, sizeof(score_str), "Final Score: %d", final_score);
    driver.drawString(90, 230, score_str, 
                     TEXT_COLOR, 
                     BG_COLOR);
    
    driver.drawString(80, 250, "Press MID to restart", 
                     TEXT_COLOR, 
                     BG_COLOR);
}

int main() {
    stdio_init_all();
    printf("Snake Game for ILI9488 - Landscape Mode\n");
    
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
    
    // 设置为竖屏模式
    lcd_driver.setRotation(ili9488::Rotation::Portrait_180);
    
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
    lcd_driver.fillScreenRGB666(BG_COLOR);
    lcd_driver.drawString(100, 220, "SNAKE GAME", 
                         TEXT_COLOR, 
                         BG_COLOR);
    lcd_driver.drawString(100, 250, "Press MID BTN to start", 
                         TEXT_COLOR, 
                         BG_COLOR);
    
    // 等待开始
    bool started = false;
    while (!started) {
        if (joystick.get_button_value() == 0) {
            started = true;
            sleep_ms(200);  // 防抖延迟
        }
        sleep_ms(JOYSTICK_LOOP_DELAY_MS);
    }
    
    // 初始化游戏状态
    GameState game_state;
    initializeGame(game_state);
    game_state.game_started = true;  // 直接开始游戏，不需要再次按键
    
    // 绘制初始游戏画面
    lcd_driver.fillScreenRGB666(BG_COLOR);
    drawBorder(lcd_driver);
    drawSnake(lcd_driver, game_state.snake);
    drawFood(lcd_driver, game_state.food);
    drawScore(lcd_driver, game_state.score);
    
    // 游戏变量
    uint32_t last_move_time = to_ms_since_boot(get_absolute_time());  // 立即开始计时
    static bool last_mid_pressed = true;  // 假设按钮刚被按下，避免立即触发
    static absolute_time_t last_red_time = {0};
    static int previous_raw_direction = 0;
    static uint8_t stable_count = 0;
    static bool is_active = false;
    static uint32_t last_displayed_countdown = 0;  // 跟踪上次显示的倒计时秒数
    
    // 主游戏循环
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // 按钮处理
        bool mid_pressed = (joystick.get_button_value() == 0);
        
        // 检测mid键按下的瞬间 - 红灯逻辑
        if (mid_pressed && !last_mid_pressed) {
            joystick.set_rgb_color(JOYSTICK_LED_RED);
            last_red_time = get_absolute_time();
            
            if (game_state.game_over || game_state.waiting_to_restart) {
                // 重新开始游戏
                initializeGame(game_state);
                game_state.game_started = true;  // 直接开始游戏
                lcd_driver.fillScreenRGB666(BG_COLOR);
                drawBorder(lcd_driver);
                drawSnake(lcd_driver, game_state.snake);
                drawFood(lcd_driver, game_state.food);
                drawScore(lcd_driver, game_state.score);
                last_move_time = current_time;
            } else if (!game_state.game_started) {
                // 开始游戏
                game_state.game_started = true;
                last_move_time = current_time;
            } else {
                // 暂停/恢复游戏
                game_state.game_paused = !game_state.game_paused;
                if (game_state.game_paused) {
                    drawPaused(lcd_driver);
                } else {
                    // 清除暂停文字并重绘可能被覆盖的游戏元素
                    clearPaused(lcd_driver);
                    
                    // 重绘可能被暂停文字覆盖的游戏元素
                    // 检查蛇身是否在暂停文字区域内并重绘
                    for (uint16_t i = 0; i < game_state.snake.length; i++) {
                        int16_t pixel_x = game_state.snake.segments[i].x * GRID_SIZE;
                        int16_t pixel_y = game_state.snake.segments[i].y * GRID_SIZE;
                        
                        // 检查是否在暂停文字区域内 (70, 220, 250, 270)
                        if (pixel_x < 250 && pixel_x + GRID_SIZE > 70 && 
                            pixel_y < 270 && pixel_y + GRID_SIZE > 220) {
                            if (i == 0) {
                                drawGridCell(lcd_driver, game_state.snake.segments[i].x, 
                                           game_state.snake.segments[i].y, SNAKE_HEAD_COLOR);
                            } else {
                                drawGridCell(lcd_driver, game_state.snake.segments[i].x, 
                                           game_state.snake.segments[i].y, SNAKE_BODY_COLOR);
                            }
                        }
                    }
                    
                    // 检查食物是否在暂停文字区域内并重绘
                    int16_t food_pixel_x = game_state.food.x * GRID_SIZE;
                    int16_t food_pixel_y = game_state.food.y * GRID_SIZE;
                    if (food_pixel_x < 250 && food_pixel_x + GRID_SIZE > 70 && 
                        food_pixel_y < 270 && food_pixel_y + GRID_SIZE > 220) {
                        drawFood(lcd_driver, game_state.food);
                    }
                }
            }
        }
        
        // 50ms后自动关闭红灯
        if (!is_nil_time(last_red_time) && absolute_time_diff_us(last_red_time, get_absolute_time()) > 50000) {
            joystick.set_rgb_color(JOYSTICK_LED_OFF);
            last_red_time = {0};
        }
        
        last_mid_pressed = mid_pressed;
        
        // 如果游戏结束，处理自动重启逻辑
        if (game_state.game_over) {
            uint32_t elapsed_time = current_time - game_state.game_over_time;
            uint32_t countdown_seconds = 5 - (elapsed_time / 1000);
            
            if (elapsed_time >= 5000) {
                // 5秒后进入等待重新开始状态
                game_state.game_over = false;
                game_state.waiting_to_restart = true;
                drawWaitingToRestart(lcd_driver, game_state.score);
                last_displayed_countdown = 0;  // 重置倒计时显示
            } else {
                // 只在倒计时秒数变化时才更新显示
                if (countdown_seconds != last_displayed_countdown) {
                    updateCountdown(lcd_driver, countdown_seconds);
                    last_displayed_countdown = countdown_seconds;
                }
            }
            sleep_ms(JOYSTICK_LOOP_DELAY_MS);
            continue;
        }
        
        // 如果在等待重新开始状态，跳过游戏逻辑
        if (game_state.waiting_to_restart) {
            sleep_ms(JOYSTICK_LOOP_DELAY_MS);
            continue;
        }
        
        // 如果游戏暂停或未开始，跳过游戏逻辑
        if (game_state.game_paused || !game_state.game_started) {
            sleep_ms(JOYSTICK_LOOP_DELAY_MS);
            continue;
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
        
        // 更新蛇的下一个方向（需要稳定的方向输入）
        if (stable_count >= 3 && raw_direction != 0) {
            game_state.snake.next_direction = static_cast<Direction>(raw_direction);
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
        
        // 游戏逻辑更新（按固定时间间隔）
        if (current_time - last_move_time >= GAME_SPEED_MS) {
            // 记录蛇尾位置（用于清除）
            Position old_tail = game_state.snake.segments[game_state.snake.length - 1];
            
            // 记录移动前的分数
            uint16_t old_score = game_state.score;
            
            // 移动蛇
            if (!moveSnake(game_state)) {
                // 游戏结束
                game_state.game_over = true;
                game_state.game_over_time = current_time;
                drawGameOver(lcd_driver, game_state.score);  // 显示完整的游戏结束画面
                last_displayed_countdown = 5;  // 初始化倒计时显示为5秒
            } else {
                // 检查是否吃到食物（通过比较分数变化）
                bool ate_food = (game_state.score > old_score);
                
                // 如果没有吃到食物，清除旧的蛇尾
                if (!ate_food) {
                    clearSnakeTail(lcd_driver, old_tail);
                }
                
                // 绘制新的蛇头
                drawGridCell(lcd_driver, game_state.snake.segments[0].x, 
                           game_state.snake.segments[0].y, SNAKE_HEAD_COLOR);
                
                // 如果蛇身长度大于1，将原来的头部变成身体
                if (game_state.snake.length > 1) {
                    drawGridCell(lcd_driver, game_state.snake.segments[1].x, 
                               game_state.snake.segments[1].y, SNAKE_BODY_COLOR);
                }
                
                // 如果吃到食物，绘制新食物和更新分数
                if (ate_food) {
                    drawFood(lcd_driver, game_state.food);
                    drawScore(lcd_driver, game_state.score);
                }
            }
            
            last_move_time = current_time;
        }
        
        sleep_ms(JOYSTICK_LOOP_DELAY_MS);
    }
    
    return 0;
}
