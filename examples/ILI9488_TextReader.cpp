/**
 * @file ILI9488_TextReader.cpp
 * @brief ILI9488 电子阅读器 - 从 SD 卡读取文本并显示
 * @version 1.0.0
 * @description 基于ILI9488 3.5寸显示屏的智能文本阅读器，支持中英文混合显示
 */

#include "ili9488_driver.hpp"
#include "pico_ili9488_gfx.hpp"
#include "ili9488_colors.hpp"
#include "joystick.hpp"
#include "hybrid_font_system.hpp"
#include "hybrid_font_renderer.hpp"
#include "rw_sd.hpp"
#include "pin_config.hpp"
#include "pico/stdlib.h"
#include <string>
#include <vector>
#include <cmath>
#include <sstream>

using namespace ili9488;
using namespace ili9488_colors;
using namespace pico_ili9488_gfx;

// 显示配置 - ILI9488 3.5寸显示屏
#define LCD_WIDTH 320
#define LCD_HEIGHT 480
#define SCREEN_MARGIN 25      // 屏幕四周留白25像素
#define SIDE_MARGIN SCREEN_MARGIN
#define TOP_MARGIN SCREEN_MARGIN
#define BOTTOM_MARGIN SCREEN_MARGIN

// 实际显示区域
#define DISPLAY_WIDTH (LCD_WIDTH - 2 * SCREEN_MARGIN)   // 270像素
#define DISPLAY_HEIGHT (LCD_HEIGHT - 2 * SCREEN_MARGIN) // 430像素

// 排版优化参数 - 专业出版标准
#define LINE_HEIGHT 24          // 行高：字体大小(16) × 1.5 = 舒适阅读
#define PARAGRAPH_SPACING 10    // 段落间额外间距
#define TITLE_CONTENT_SPACING 15 // 标题与内容间距
#define CONTENT_FOOTER_SPACING 25 // 内容与页脚间距

// 默认文本文件路径
#define TEXT_FILE_PATH "/Stone.txt"

class ILI9488TextReader {
private:
    ILI9488Driver display_;
    PicoILI9488GFX<ILI9488Driver> gfx_;
    Joystick joystick_;
    hybrid_font::FontManager<ILI9488Driver> font_manager_;
    MicroSD::RWSD sd_;
    
    int current_page_;
    int total_pages_;
    std::string filename_;
    std::vector<std::string> current_page_content_;
    bool sd_ready_;
    size_t file_position_;
    size_t file_size_;
    
    // 存储每页的起始字节位置
    std::vector<size_t> page_start_positions_;
    
    // 从文件路径中提取文件名 (静态函数)
    static std::string extract_filename_from_path(const std::string& path) {
        std::string full_path = path;
        // 找到最后一个 '/' 或 '\' 字符
        size_t pos = full_path.find_last_of("/\\");
        if (pos != std::string::npos) {
            return full_path.substr(pos + 1);
        }
        return full_path; // 如果没有路径分隔符，返回整个字符串
    }
    
    // 方向判断
    int determine_joystick_direction(int16_t x, int16_t y) {
        int16_t abs_x = std::abs(x);
        int16_t abs_y = std::abs(y);
        if (abs_y > abs_x * 1.2 && abs_y > JOYSTICK_DEADZONE) {
            return (y < 0) ? 1 : 2;  // 1=up, 2=down
        }
        if (abs_x > abs_y * 1.2 && abs_x > JOYSTICK_DEADZONE) {
            return (x < 0) ? 3 : 4;  // 3=left, 4=right
        }
        return 0;  // center
    }

    // 等待摇杆回中
    void wait_joystick_center() {
        while (true) {
            int16_t x = joystick_.get_joy_adc_12bits_offset_value_x();
            int16_t y = joystick_.get_joy_adc_12bits_offset_value_y();
            if (std::abs(x) < JOYSTICK_DEADZONE && std::abs(y) < JOYSTICK_DEADZONE) break;
            sleep_ms(10);
        }
    }

    void initialize_hardware() {
        printf("初始化 ILI9488 显示屏...\n");
        
        if (!display_.initialize()) {
            printf("[ERROR] 显示屏初始化失败\n");
            joystick_.set_rgb_color(JOYSTICK_LED_RED);
            sleep_ms(2000);
            return;
        }
        
        // 设置屏幕180度旋转
        display_.setRotation(Rotation::Portrait_180);
        printf("屏幕已设置为180度旋转\n");
        
        // 设置背光
        display_.setBacklight(true);
        display_.setBacklightBrightness(255);
        
        printf("显示屏初始化完成.\n");
        
        // 清除显示屏
        display_.fillScreenRGB666(0x000000);
        
        // 初始化 joystick
        printf("初始化摇杆控制器...\n");
        if (!joystick_.begin(JOYSTICK_GET_I2C_CONFIG())) {
            printf("[ERROR] 摇杆控制器初始化失败\n");
            joystick_.set_rgb_color(JOYSTICK_LED_RED);
            sleep_ms(2000);
        } else {
            printf("摇杆控制器初始化完成.\n");
        }
        
        // 初始化混合字体系统
        if (!font_manager_.initialize()) {
            printf("[ERROR] 混合字体系统初始化失败\n");
            joystick_.set_rgb_color(JOYSTICK_LED_RED);
            sleep_ms(2000);
        } else {
            printf("[SUCCESS] 混合字体系统初始化成功\n");
            font_manager_.print_status();
            joystick_.set_rgb_color(JOYSTICK_LED_GREEN);
            sleep_ms(1000);
        }
        
        joystick_.set_rgb_color(JOYSTICK_LED_OFF);
        printf("[INFO] 显示系统初始化完成 (180度旋转)\n");
    }

    bool initialize_microsd() {
        printf("\n===== 初始化 MicroSD 卡 =====\n");
        
        // 显示引脚配置信息
        printf("MicroSD 引脚配置:\n");
        printf("  MISO: GPIO %d\n", MICROSD_PIN_MISO);
        printf("  MOSI: GPIO %d\n", MICROSD_PIN_MOSI);
        printf("  SCK:  GPIO %d\n", MICROSD_PIN_SCK);
        printf("  CS:   GPIO %d\n", MICROSD_PIN_CS);
        printf("  SPI:  %s\n", (SPI_PORT_MICROSD == spi0) ? "spi0" : "spi1");
        
        // 使用MicroSD配置结构体
        MicroSD::SPIConfig config = MicroSD::Config::DEFAULT;
        printf("使用配置: %s\n", config.get_description().c_str());
        
        // 初始化SD卡
        printf("开始初始化 SD 卡...\n");
        auto init_result = sd_.initialize();
        if (!init_result.is_ok()) {
            printf("[ERROR] SD卡初始化失败: %s\n", MicroSD::StorageDevice::get_error_description(init_result.error_code()).c_str());
            printf("可能的原因:\n");
            printf("  1. SD 卡未插入或接触不良\n");
            printf("  2. 引脚连接错误\n");
            printf("  3. SD 卡格式不支持（需要 FAT32）\n");
            printf("  4. 电源供应不稳定\n");
            return false;
        }
        
        printf("[SUCCESS] SD卡初始化成功!\n");
        
        // 显示状态信息
        printf("%s", sd_.get_status_info().c_str());
        printf("%s", sd_.get_config_info().c_str());
        
        return true;
    }

    bool initialize_file_info() {
        printf("\n===== 初始化文件信息 =====\n");
        
        // 检查文件是否存在
        if (!sd_.file_exists(TEXT_FILE_PATH)) {
            printf("文件不存在: %s\n", TEXT_FILE_PATH);
            return false;
        }
        
        printf("文件: %s\n", TEXT_FILE_PATH);
        
        // 获取文件大小
        auto file_info = sd_.get_file_info(TEXT_FILE_PATH);
        if (!file_info.is_ok()) {
            printf("获取文件信息失败\n");
            return false;
        }
        
        file_size_ = file_info->size;
        file_position_ = 0;
        
        printf("[SUCCESS] 文件信息获取成功\n");
        printf("  文件大小: %zu 字节\n", file_size_);
        
        return true;
    }

    // 预扫描文件，计算每页的准确起始位置
    bool precalculate_page_positions() {
        printf("[预扫描] 开始计算每页起始位置...\n");
        
        page_start_positions_.clear();
        page_start_positions_.push_back(0);  // 第0页从0开始
        
        // 打开文件
        auto file_handle = sd_.open_file(TEXT_FILE_PATH, "r");
        if (!file_handle.is_ok()) {
            printf("打开文件失败: %s\n", MicroSD::StorageDevice::get_error_description(file_handle.error_code()).c_str());
            return false;
        }
        
        auto& handle = *file_handle;
        
        // 计算每页可显示的行数
        int content_start_y = TOP_MARGIN + 20 + TITLE_CONTENT_SPACING;
        int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - CONTENT_FOOTER_SPACING;
        int content_height = content_end_y - content_start_y;
        int max_display_lines_per_page = content_height / LINE_HEIGHT;
        max_display_lines_per_page = max_display_lines_per_page * 85 / 100;
        
        printf("[预扫描] 每页最多显示 %d 行\n", max_display_lines_per_page);
        
        const size_t BUFFER_SIZE = 2048;
        std::string accumulated_text;
        std::vector<std::string> current_page_lines;
        size_t current_position = 0;
        int current_page = 0;
        
        while (true) {
            auto read_result = handle.read(BUFFER_SIZE);
            if (!read_result.is_ok()) {
                printf("[ERROR] 读取文件失败\n");
                handle.close();
                return false;
            }
            
            auto& data = *read_result;
            if (data.empty()) {
                break; // 文件结束
            }
            
            std::string text_chunk(data.begin(), data.end());
            accumulated_text += text_chunk;
            
            // 处理累积的文本，按行分割
            size_t pos = 0;
            while (pos < accumulated_text.size()) {
                size_t newline_pos = accumulated_text.find('\n', pos);
                if (newline_pos == std::string::npos) {
                    // 没有完整行，保留剩余文本等待下次读取
                    accumulated_text = accumulated_text.substr(pos);
                    break;
                }
                
                std::string line = accumulated_text.substr(pos, newline_pos - pos);
                
                // 对每一行进行智能换行处理
                std::vector<std::string> wrapped_lines = wrap_text_lines(line, DISPLAY_WIDTH);
                
                // 添加换行后的行到当前页
                for (const std::string& wrapped_line : wrapped_lines) {
                    current_page_lines.push_back(wrapped_line);
                    
                    // 如果当前页已满，记录下一页的起始位置
                    if (current_page_lines.size() >= static_cast<size_t>(max_display_lines_per_page)) {
                        // 计算当前行的结束位置
                        size_t line_end_pos = current_position + (newline_pos - pos) + 1; // +1 for newline
                        page_start_positions_.push_back(line_end_pos);
                        current_page++;
                        
                        printf("[预扫描] 第 %d 页结束位置: %zu 字节，包含 %zu 行\n", 
                               current_page, line_end_pos, current_page_lines.size());
                        
                        // 清空当前页，开始下一页
                        current_page_lines.clear();
                    }
                }
                
                current_position += (newline_pos - pos) + 1; // +1 for newline
                pos = newline_pos + 1;
            }
        }
        
        // 处理最后的文本
        if (!accumulated_text.empty()) {
            std::vector<std::string> wrapped_lines = wrap_text_lines(accumulated_text, DISPLAY_WIDTH);
            for (const std::string& wrapped_line : wrapped_lines) {
                current_page_lines.push_back(wrapped_line);
            }
        }
        
        // 如果还有剩余内容，添加最后一页
        if (!current_page_lines.empty()) {
            page_start_positions_.push_back(file_size_);
            printf("[预扫描] 最后一页结束位置: %zu 字节，包含 %zu 行\n", 
                   file_size_, current_page_lines.size());
        }
        
        handle.close();
        
        total_pages_ = page_start_positions_.size() - 1; // 减1因为起始位置比页数多1
        printf("[预扫描] 完成！总页数: %d\n", total_pages_);
        
        return true;
    }
    
    bool load_page_content(int page_num) {
        printf("[加载页面] 正在加载第 %d 页内容...\n", page_num + 1);
        
        // 检查页面范围
        if (page_num < 0 || page_num >= total_pages_ || page_num >= static_cast<int>(page_start_positions_.size() - 1)) {
            printf("[ERROR] 页面号超出范围: %d (总页数: %d)\n", page_num, total_pages_);
            return false;
        }
        
        // 打开文件
        auto file_handle = sd_.open_file(TEXT_FILE_PATH, "r");
        if (!file_handle.is_ok()) {
            printf("打开文件失败: %s\n", MicroSD::StorageDevice::get_error_description(file_handle.error_code()).c_str());
            return false;
        }
        
        auto& handle = *file_handle;
        
        // 使用预计算的起始位置
        size_t start_pos = page_start_positions_[page_num];
        size_t end_pos = page_start_positions_[page_num + 1];
        
        printf("[加载] 页面 %d: 从 %zu 到 %zu 字节 (共 %zu 字节)\n", 
               page_num + 1, start_pos, end_pos, end_pos - start_pos);
        
        // 设置文件位置
        auto seek_result = handle.seek(start_pos);
        if (!seek_result.is_ok()) {
            printf("[ERROR] 文件定位失败\n");
            handle.close();
            return false;
        }
        
        current_page_content_.clear();
        
        const size_t BUFFER_SIZE = 1024;
        std::string accumulated_text;
        size_t bytes_read = 0;
        
        // 读取当前页的内容
        while (bytes_read < (end_pos - start_pos)) {
            size_t remaining_bytes = end_pos - start_pos - bytes_read;
            size_t read_size = std::min(BUFFER_SIZE, remaining_bytes);
            
            auto read_result = handle.read(read_size);
            if (!read_result.is_ok()) {
                printf("[ERROR] 读取文件失败\n");
                handle.close();
                return false;
            }
            
            auto& data = *read_result;
            if (data.empty()) {
                break; // 文件结束
            }
            
            std::string text_chunk(data.begin(), data.end());
            accumulated_text += text_chunk;
            bytes_read += data.size();
        }
        
        // 处理读取的内容，按行分割并换行
        size_t pos = 0;
        while (pos < accumulated_text.size()) {
            size_t newline_pos = accumulated_text.find('\n', pos);
            if (newline_pos == std::string::npos) {
                // 处理最后一行（可能没有换行符）
                std::string line = accumulated_text.substr(pos);
                if (!line.empty()) {
                    std::vector<std::string> wrapped_lines = wrap_text_lines(line, DISPLAY_WIDTH);
                    for (const std::string& wrapped_line : wrapped_lines) {
                        current_page_content_.push_back(wrapped_line);
                    }
                }
                break;
            }
            
            std::string line = accumulated_text.substr(pos, newline_pos - pos);
            
            // 对每一行进行智能换行处理
            std::vector<std::string> wrapped_lines = wrap_text_lines(line, DISPLAY_WIDTH);
            for (const std::string& wrapped_line : wrapped_lines) {
                current_page_content_.push_back(wrapped_line);
            }
            
            pos = newline_pos + 1;
        }
        
        handle.close();
        
        printf("[SUCCESS] 第 %d 页加载完成，包含 %zu 行\n", page_num + 1, current_page_content_.size());
        return true;
    }

    void draw_header() {
        // 显示文件名 - 专业排版：适当的顶部留白
        font_manager_.draw_string(display_, SIDE_MARGIN, SIDE_MARGIN - 5, filename_, true);
        
        // 绘制分割线 - 与内容保持合适距离
        int separator_y = SIDE_MARGIN + 15;
        gfx_.drawFastHLine(SIDE_MARGIN, separator_y, LCD_WIDTH - 2 * SIDE_MARGIN, 0xFFFF);
    }

    // 页脚显示，支持提示
    void draw_footer(int current_page, const std::string& tip = "") {
        // 确保页码信息有效
        if (total_pages_ <= 0) return;
        
        std::string page_info = "Page " + std::to_string(current_page + 1) + "/" + std::to_string(total_pages_);
        int text_width = font_manager_.get_string_width(page_info);
        int footer_y = LCD_HEIGHT - BOTTOM_MARGIN - 15;
        
        // 确保页脚位置在有效范围内
        if (footer_y > 0 && footer_y < LCD_HEIGHT) {
            font_manager_.draw_string(display_, (LCD_WIDTH - text_width) / 2, footer_y, page_info, true);
        }
        
        if (!tip.empty() && tip.size() > 0) {
            int tip_width = font_manager_.get_string_width(tip);
            int tip_y = footer_y - 20;  // 在页码上方显示提示
            if (tip_y > 0 && tip_y < LCD_HEIGHT) {
                font_manager_.draw_string(display_, (LCD_WIDTH - tip_width) / 2, tip_y, tip, true);
            }
        }
    }

    // 判断是否为中文字符（UTF-8编码）
    bool is_chinese_char(const std::string& text, size_t pos) {
        if (pos >= text.size()) return false;
        unsigned char c = text[pos];
        // UTF-8中文字符的第一个字节通常在0xE0-0xEF范围内
        return (c >= 0xE0 && c <= 0xEF);
    }
    
    // 获取UTF-8字符的字节长度
    int get_utf8_char_length(const std::string& text, size_t pos) {
        if (pos >= text.size()) return 0;
        unsigned char c = text[pos];
        if (c < 0x80) return 1;        // ASCII字符
        else if (c < 0xE0) return 2;   // 2字节UTF-8
        else if (c < 0xF0) return 3;   // 3字节UTF-8（大部分中文）
        else return 4;                 // 4字节UTF-8
    }
    
    // 智能换行：将文本分割成适合显示宽度的行（支持中英文混合）
    std::vector<std::string> wrap_text_lines(const std::string& text, int max_width) {
        std::vector<std::string> lines;
        
        if (text.empty()) {
            lines.push_back("");  // 空行用空字符串表示
            return lines;
        }
        
        std::string current_line;
        size_t pos = 0;
        
        while (pos < text.size()) {
            if (is_chinese_char(text, pos)) {
                // 处理中文字符：逐个字符添加
                int char_len = get_utf8_char_length(text, pos);
                std::string chinese_char = text.substr(pos, char_len);
                
                // 测试加上这个中文字符后的宽度
                std::string test_line = current_line + chinese_char;
                int test_width = font_manager_.get_string_width(test_line);
                
                if (test_width <= max_width) {
                    // 可以放在当前行
                    current_line = test_line;
                } else {
                    // 放不下，需要换行
                    if (!current_line.empty()) {
                        lines.push_back(current_line);
                        current_line = chinese_char;
                    } else {
                        // 当前行为空，强制放入
                        current_line = chinese_char;
                    }
                }
                
                pos += char_len;
            } else {
                // 处理英文单词：按空格分割
                size_t next_space = text.find(' ', pos);
                size_t word_end = (next_space == std::string::npos) ? text.size() : next_space;
                
                // 检查是否遇到中文字符
                for (size_t i = pos; i < word_end; i++) {
                    if (is_chinese_char(text, i)) {
                        word_end = i;
                        break;
                    }
                }
                
                std::string word = text.substr(pos, word_end - pos);
                
                // 计算加上这个英文单词后的行宽度
                std::string test_line = current_line;
                if (!test_line.empty() && !word.empty() && word[0] != ' ') {
                    test_line += " ";
                }
                test_line += word;
                
                int test_width = font_manager_.get_string_width(test_line);
                
                if (test_width <= max_width) {
                    // 这个词可以放在当前行
                    current_line = test_line;
                } else {
                    // 这个词放不下，需要换行 - 绝不截断
                    if (!current_line.empty()) {
                        lines.push_back(current_line);
                        current_line = word;
                    } else {
                        // 当前行为空但词太长，仍然完整放入 - 绝不截断
                        current_line = word;
                    }
                }
                
                // 移动到下一个位置
                pos = word_end;
                if (pos < text.size() && text[pos] == ' ') {
                    pos++;  // 跳过空格
                }
            }
        }
        
        // 添加最后一行
        if (!current_line.empty()) {
            lines.push_back(current_line);
        }
        
        return lines;
    }

    void show_static_page(int page, const std::string& tip = "") {
        // 清屏
        display_.fillScreenRGB666(0x000000);
        
        draw_header();
        
        // 专业排版：计算内容显示区域
        int content_start_y = TOP_MARGIN + 20 + TITLE_CONTENT_SPACING;  // 标题下方，增加间距
        int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - CONTENT_FOOTER_SPACING;  // 页脚上方，增加间距
        
        int y = content_start_y;
        bool prev_line_empty = false;
        int lines_drawn = 0;
        
        // 专业排版：智能绘制文本（支持段落间距）
        // current_page_content_ 已经包含了换行后的行，直接使用
        for (size_t i = 0; i < current_page_content_.size() && y < content_end_y - LINE_HEIGHT; i++) {
            const std::string& line_text = current_page_content_[i];
            bool current_line_empty = line_text.empty() || line_text.size() == 0;
            
            // 段落间距处理：连续空行只显示一个，并增加段落间距
            if (current_line_empty) {
                if (!prev_line_empty) {
                    // 第一个空行：增加段落间距
                    y += PARAGRAPH_SPACING;
                    prev_line_empty = true;
                }
                // 跳过连续的空行
                continue;
            }
            
            // 绘制非空行
            font_manager_.draw_string(display_, SIDE_MARGIN, y, line_text, true);
            y += LINE_HEIGHT;
            lines_drawn++;
            prev_line_empty = false;
            
            // 防止超出页面
            if (y > content_end_y - LINE_HEIGHT) {
                break;
            }
        }
        
        printf("[显示] 第 %d 页绘制了 %d 行文本\n", page + 1, lines_drawn);
        draw_footer(page, tip);
    }

    int estimate_total_pages() {
        // 专业排版：重新计算每页可显示的行数
        int content_start_y = TOP_MARGIN + 20 + TITLE_CONTENT_SPACING;
        int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - CONTENT_FOOTER_SPACING;
        int content_height = content_end_y - content_start_y;
        
        // 考虑新的行高和段落间距
        int max_lines_per_page = content_height / LINE_HEIGHT;
        
        // 减少估算行数以考虑段落间距占用的空间
        max_lines_per_page = max_lines_per_page * 85 / 100;  // 减少15%来考虑段落间距
        
        // 基于文件大小进行估算
        // 假设平均每行约50个字符（考虑更舒适的排版）
        const size_t avg_chars_per_line = 50;
        size_t estimated_total_lines = file_size_ / avg_chars_per_line;
        
        // 考虑智能换行，乘以1.4的系数（略微保守）
        estimated_total_lines = estimated_total_lines * 7 / 5;
        
        int estimated_pages = (estimated_total_lines + max_lines_per_page - 1) / max_lines_per_page;
        
        printf("[排版优化] 文件大小: %zu 字节，每页约 %d 行，估算页数: %d\n", 
               file_size_, max_lines_per_page, estimated_pages);
        
        return estimated_pages > 0 ? estimated_pages : 1;
    }
    
    void display_error_screen(const std::string& error_msg) {
        // 清屏
        display_.fillScreenRGB666(0x000000);
        
        draw_header();
        
        // 居中显示错误图标和标题
        int center_x = LCD_WIDTH / 2;
        int y = LCD_HEIGHT / 2 - 80;
        
        std::string error_title = "❌ 系统错误";
        int title_width = font_manager_.get_string_width(error_title);
        font_manager_.draw_string(display_, center_x - title_width/2, y, error_title, true);
        
        y += LINE_HEIGHT * 2;
        
        // 错误框架
        int box_width = 280;
        int box_height = 120;
        int box_x = center_x - box_width/2;
        int box_y = y;
        
        // 绘制错误框
        gfx_.drawRect(box_x, box_y, box_width, box_height, 0xFFFF);
        
        // 显示错误消息 - 完整显示不截断
        y += 20;
        int max_msg_width = box_width - 20;
        
        // 使用智能换行处理错误消息，而不是截断
        std::vector<std::string> error_lines = wrap_text_lines(error_msg, max_msg_width);
        
        for (const std::string& line : error_lines) {
            if (y > box_y + box_height - 25) break;  // 避免超出框框
            
            int line_width = font_manager_.get_string_width(line);
            font_manager_.draw_string(display_, center_x - line_width/2, y, line, true);
            y += LINE_HEIGHT;
        }
        
        // 建议信息
        y = box_y + box_height + 20;
        std::string suggestion = "请检查 SD 卡连接和格式";
        int sug_width = font_manager_.get_string_width(suggestion);
        font_manager_.draw_string(display_, center_x - sug_width/2, y, suggestion, true);
        
        y += LINE_HEIGHT;
        std::string suggestion2 = "Check SD card connection";
        int sug2_width = font_manager_.get_string_width(suggestion2);
        font_manager_.draw_string(display_, center_x - sug2_width/2, y, suggestion2, true);
        
        // 底部重试提示
        y = LCD_HEIGHT - BOTTOM_MARGIN - 35;
        std::string retry_msg = "程序将在 5 秒后结束";
        int retry_width = font_manager_.get_string_width(retry_msg);
        font_manager_.draw_string(display_, center_x - retry_width/2, y, retry_msg, true);
        
        // 显示5秒后退出，不再无限循环
        for (int i = 5; i > 0; i--) {
            sleep_ms(1000);
            printf("[错误] %s - %d 秒后程序退出\n", error_msg.c_str(), i);
        }
    }

public:
    void run() {
        printf("\n===== 开始 ILI9488 电子阅读器 =====\n");
        
        // 初始化 MicroSD 卡
        sd_ready_ = initialize_microsd();
        
        if (!sd_ready_) {
            printf("[ERROR] SD 卡初始化失败，尝试重试最多3次...\n");
            // 有限次数重试 SD 卡初始化
            for (int retry = 1; retry <= 3; retry++) {
                printf("第 %d 次重试 SD 卡初始化...\n", retry);
                sleep_ms(2000);
                sd_ready_ = initialize_microsd();
                if (sd_ready_) {
                    printf("[SUCCESS] SD 卡重试初始化成功！\n");
                    break;
                }
                printf("第 %d 次重试失败\n", retry);
            }
        }
        
        if (!sd_ready_) {
            printf("[FAILED] SD卡初始化失败。\n");
            printf("请检查SD卡连接、格式和引脚配置。\n");
            // 显示错误信息
            display_error_screen("SD卡初始化失败");
            return;
        }
        
        // 初始化文件信息
        printf("\n===== 初始化文件信息 =====\n");
        bool init_success = initialize_file_info();
        
        if (!init_success) {
            printf("[PARTIAL] SD卡初始化成功，但文件信息获取失败。\n");
            printf("请检查文件 '%s' 是否存在于SD卡根目录。\n", TEXT_FILE_PATH);
            // 显示错误信息
            display_error_screen("文件读取失败");
            return;
        }
        
        // 开始分页显示模式
        printf("\n===== 进入分页显示模式 =====\n");
        current_page_ = 0;
        
        // 预扫描文件，计算每页的准确起始位置
        bool precalc_success = precalculate_page_positions();
        
        if (!precalc_success) {
            printf("[ERROR] 预扫描失败，无法继续加载页面。\n");
            display_error_screen("预扫描失败");
            return;
        }
        
        // 加载第一页内容
        if (!load_page_content(current_page_)) {
            printf("[ERROR] 加载第一页失败\n");
            display_error_screen("页面加载失败");
            return;
        }
        
        // 专业排版：计算实际每页行数
        int content_start_y = TOP_MARGIN + 20 + TITLE_CONTENT_SPACING;
        int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - CONTENT_FOOTER_SPACING;
        int content_height = content_end_y - content_start_y;
        int max_lines_per_page = (content_height / LINE_HEIGHT) * 85 / 100;
        
        printf("[INFO] 显示配置: 屏幕留白 %d 像素，显示区域 %dx%d 像素\n", 
               SCREEN_MARGIN, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        printf("[INFO] 页面配置: 每页最多 %d 行，总共 %d 页\n", max_lines_per_page, total_pages_);
        printf("[INFO] 摇杆控制: 上下翻页，按钮切换显示模式\n");
        
        show_static_page(current_page_);
        
        // 主控制循环 - 基于 joystick 控制
        while (true) {
            int16_t x = joystick_.get_joy_adc_12bits_offset_value_x();
            int16_t y = joystick_.get_joy_adc_12bits_offset_value_y();
            bool button_pressed = joystick_.get_button_value();
            
            int direction = determine_joystick_direction(x, y);
            
            if (direction == 1) { // 上 - 上一页
                if (current_page_ > 0) {
                    current_page_--;
                    if (load_page_content(current_page_)) {
                        show_static_page(current_page_);
                        printf("[翻页] 上一页: %d/%d\n", current_page_ + 1, total_pages_);
                    } else {
                        printf("[错误] 加载上一页失败\n");
                        current_page_++; // 恢复页码
                        show_static_page(current_page_, "加载失败");
                    }
                } else {
                    show_static_page(current_page_, "已到首页");
                    printf("[提示] 已到首页\n");
                }
                wait_joystick_center();
            } else if (direction == 2) { // 下 - 下一页
                // 尝试加载下一页，如果成功就翻页，失败就表示到末页了
                int next_page = current_page_ + 1;
                if (load_page_content(next_page)) {
                    current_page_ = next_page;
                    show_static_page(current_page_);
                    printf("[翻页] 下一页: %d/%d+\n", current_page_ + 1, total_pages_);
                } else {
                    show_static_page(current_page_, "已到末页");
                    printf("[提示] 已到末页\n");
                }
                wait_joystick_center();
            }
            
            // 按键切换显示模式（这里可以添加其他功能）
            static bool last_button_state = false;
            if (button_pressed && !last_button_state) {
                printf("[模式] 按钮按下 - 可以添加功能\n");
                // 这里可以添加按钮功能，比如切换字体大小、显示模式等
                wait_joystick_center();
            }
            last_button_state = button_pressed;
            
            sleep_ms(30);  // 主循环延迟
        }
    }

    ILI9488TextReader() : 
        display_(ILI9488_GET_SPI_CONFIG()),
        gfx_(display_, LCD_WIDTH, LCD_HEIGHT),
        font_manager_(),
        sd_(),
        current_page_(0),
        filename_(extract_filename_from_path(TEXT_FILE_PATH)),
        sd_ready_(false),
        file_position_(0),
        file_size_(0) {
        
        // 初始化显示系统
        initialize_hardware();
    }
};

int main() {
    // 初始化串口
    stdio_init_all();
    sleep_ms(3000); // 延长等待时间确保串口连接稳定
    
    // 输出启动信息
    printf("\n\n===== 程序启动 =====\n");
    printf("ILI9488 摇杆控制电子阅读器\n");
    printf("目标文件: '%s'\n", TEXT_FILE_PATH);
    printf("功能特性: 摇杆控制分页显示\n");
    printf("显示方式: 静态分页，摇杆控制翻页\n");
    printf("控制方式: 摇杆上下翻页，按钮预留功能\n");
    printf("输出方式: 屏幕显示 + 串口日志\n");
    printf("特点: 支持中英文混合显示，智能换行\n");
    printf("===================================\n");
    
    // 系统信息
    printf("\n[INFO] 系统启动完成\n");
    printf("[INFO] 开始创建 ILI9488TextReader 对象...\n");
    
    ILI9488TextReader reader;
    printf("[INFO] 对象创建成功，开始运行测试...\n");
    reader.run();
    printf("[INFO] 测试运行完成\n");
    
    printf("[INFO] 程序即将退出\n");
    return 0;
} 