/**
 * @file ili9488_hal.c
 * @brief ILI9488 LCD驱动硬件抽象层实现
 */

#include <stdio.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "ili9488_hal.h"

// 硬件配置
static struct {
    bool is_initialized;
    spi_inst_t *spi;
    
    // 引脚
    uint8_t pin_din;
    uint8_t pin_sck;
    uint8_t pin_cs;
    uint8_t pin_dc;
    uint8_t pin_reset;
    uint8_t pin_bl;
    
    // PWM相关
    uint slice_num;         // 背光PWM的slice编号
    uint channel;           // 背光PWM的通道
    bool pwm_enabled;       // PWM是否已启用
} hw_config = {0};

bool ili9488_hal_init(const ili9488_hw_config_t *config) {
    if (config == NULL) {
        printf("Error: NULL configuration\n");
        return false;
    }
    
    // 保存配置
    hw_config.spi = config->spi_inst;
    hw_config.pin_din = config->pin_din;
    hw_config.pin_sck = config->pin_sck;
    hw_config.pin_cs = config->pin_cs;
    hw_config.pin_dc = config->pin_dc;
    hw_config.pin_reset = config->pin_reset;
    hw_config.pin_bl = config->pin_bl;
    
    printf("Initializing ILI9488 hardware...\n");
    
    // 初始化GPIO引脚
    gpio_init(hw_config.pin_dc);
    gpio_init(hw_config.pin_cs);
    gpio_init(hw_config.pin_reset);
    gpio_init(hw_config.pin_bl);
    
    // 设置GPIO方向
    gpio_set_dir(hw_config.pin_dc, GPIO_OUT);
    gpio_set_dir(hw_config.pin_cs, GPIO_OUT);
    gpio_set_dir(hw_config.pin_reset, GPIO_OUT);
    gpio_set_dir(hw_config.pin_bl, GPIO_OUT);
    
    // 设置初始状态
    gpio_put(hw_config.pin_cs, 1);     // 默认不选中
    gpio_put(hw_config.pin_dc, 1);     // 默认数据模式
    gpio_put(hw_config.pin_reset, 1);  // 默认不复位
    gpio_put(hw_config.pin_bl, 0);     // 默认背光关闭
    
    // 初始化SPI
    spi_init(hw_config.spi, config->spi_speed_hz);
    
    // 设置SPI格式 - 8位，模式0，MSB优先
    spi_set_format(hw_config.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    // 设置SPI引脚
    gpio_set_function(hw_config.pin_sck, GPIO_FUNC_SPI);
    gpio_set_function(hw_config.pin_din, GPIO_FUNC_SPI);
    
    // 初始化PWM背光控制
    // 获取背光引脚对应的PWM slice和通道
    hw_config.slice_num = pwm_gpio_to_slice_num(hw_config.pin_bl);
    hw_config.channel = pwm_gpio_to_channel(hw_config.pin_bl);
    
    // 配置GPIO为PWM功能
    gpio_set_function(hw_config.pin_bl, GPIO_FUNC_PWM);
    
    // 配置PWM 
    // 使用125MHz / 65535 ≈ 1.9kHz的PWM频率
    pwm_set_wrap(hw_config.slice_num, 65535);
    pwm_set_chan_level(hw_config.slice_num, hw_config.channel, 0);  // 初始化为0（关闭）
    pwm_set_enabled(hw_config.slice_num, true);
    
    hw_config.pwm_enabled = true;
    
    // 执行硬件复位
    ili9488_hal_reset();
    
    hw_config.is_initialized = true;
    printf("ILI9488 hardware initialization completed\n");
    
    return true;
}

void ili9488_hal_reset(void) {
    if (!hw_config.is_initialized) {
        return;
    }
    
    printf("Performing hardware reset...\n");
    
    // 硬件复位序列
    gpio_put(hw_config.pin_reset, 1);
    ili9488_hal_delay_ms(10);
    gpio_put(hw_config.pin_reset, 0);
    ili9488_hal_delay_ms(20);
    gpio_put(hw_config.pin_reset, 1);
    ili9488_hal_delay_ms(120);
}

void ili9488_hal_write_cmd(uint8_t cmd) {
    if (!hw_config.is_initialized) {
        return;
    }
    
    gpio_put(hw_config.pin_cs, 0);  // 选中芯片
    gpio_put(hw_config.pin_dc, 0);  // 命令模式
    spi_write_blocking(hw_config.spi, &cmd, 1);
    gpio_put(hw_config.pin_cs, 1);  // 取消选中
}

void ili9488_hal_write_data(uint8_t data) {
    if (!hw_config.is_initialized) {
        return;
    }
    
    gpio_put(hw_config.pin_cs, 0);  // 选中芯片
    gpio_put(hw_config.pin_dc, 1);  // 数据模式
    spi_write_blocking(hw_config.spi, &data, 1);
    gpio_put(hw_config.pin_cs, 1);  // 取消选中
}

void ili9488_hal_write_data_buffer(const uint8_t *data, size_t len) {
    if (!hw_config.is_initialized || data == NULL || len == 0) {
        return;
    }
    
    gpio_put(hw_config.pin_cs, 0);  // 选中芯片
    gpio_put(hw_config.pin_dc, 1);  // 数据模式
    spi_write_blocking(hw_config.spi, data, len);
    gpio_put(hw_config.pin_cs, 1);  // 取消选中
}

void ili9488_hal_set_backlight(bool on) {
    if (!hw_config.is_initialized) {
        return;
    }
    
    if (hw_config.pwm_enabled) {
        // 如果PWM已启用，设置为最大亮度或关闭
        pwm_set_chan_level(hw_config.slice_num, hw_config.channel, on ? 65535 : 0);
    } else {
        // 否则使用GPIO控制
        gpio_put(hw_config.pin_bl, on ? 1 : 0);
    }
}

void ili9488_hal_set_backlight_brightness(uint8_t brightness) {
    if (!hw_config.is_initialized) {
        return;
    }
    
    if (hw_config.pwm_enabled) {
        // 将8位亮度值(0-255)映射到16位PWM范围(0-65535)
        uint16_t level = (uint32_t)brightness * 65535 / 255;
        pwm_set_chan_level(hw_config.slice_num, hw_config.channel, level);
    } else {
        // 如果PWM未启用，则任何非零亮度值都将开启背光
        gpio_put(hw_config.pin_bl, brightness > 0 ? 1 : 0);
    }
}

void ili9488_hal_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

void ili9488_hal_delay_us(uint32_t us) {
    sleep_us(us);
} 