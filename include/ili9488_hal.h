/**
 * @file ili9488_hal.h
 * @brief ILI9488 LCD驱动硬件抽象层
 */

#ifndef _ILI9488_HAL_H_
#define _ILI9488_HAL_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"

/**
 * @brief LCD硬件配置结构体
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
} ili9488_hw_config_t;

/**
 * @brief 初始化硬件
 * 
 * @param config 硬件配置参数
 * @return bool 初始化是否成功
 */
bool ili9488_hal_init(const ili9488_hw_config_t *config);

/**
 * @brief 进行硬件复位
 */
void ili9488_hal_reset(void);

/**
 * @brief 发送命令
 * 
 * @param cmd 命令字节
 */
void ili9488_hal_write_cmd(uint8_t cmd);

/**
 * @brief 发送单个数据字节
 * 
 * @param data 数据字节
 */
void ili9488_hal_write_data(uint8_t data);

/**
 * @brief 发送多个数据字节
 * 
 * @param data 数据缓冲区指针
 * @param len 数据长度
 */
void ili9488_hal_write_data_buffer(const uint8_t *data, size_t len);

/**
 * @brief 设置背光状态
 * 
 * @param on 背光状态(true为开,false为关)
 */
void ili9488_hal_set_backlight(bool on);

/**
 * @brief 设置背光亮度
 * 
 * @param brightness 亮度等级(0-255)，0为关闭，255为最亮
 */
void ili9488_hal_set_backlight_brightness(uint8_t brightness);

/**
 * @brief 延时毫秒
 * 
 * @param ms 毫秒数
 */
void ili9488_hal_delay_ms(uint32_t ms);

/**
 * @brief 延时微秒
 * 
 * @param us 微秒数
 */
void ili9488_hal_delay_us(uint32_t us);

#endif /* _ILI9488_HAL_H_ */ 