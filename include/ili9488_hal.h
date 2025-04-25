/**
 * @file ili9488_hal.h
 * @brief Hardware Abstraction Layer for ILI9488 LCD driver
 */

#ifndef _ILI9488_HAL_H_
#define _ILI9488_HAL_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"

/**
 * @brief LCD hardware configuration structure
 */
typedef struct {
    spi_inst_t *spi_inst;    // SPI instance
    uint32_t spi_speed_hz;   // SPI speed (Hz)
    
    // Pin definitions
    uint8_t pin_din;         // MOSI pin
    uint8_t pin_sck;         // SCK pin
    uint8_t pin_cs;          // CS pin
    uint8_t pin_dc;          // Data/Command pin
    uint8_t pin_reset;       // Reset pin
    uint8_t pin_bl;          // Backlight pin
    
    // DMA configuration
    bool use_dma;            // Whether to use DMA transfer
} ili9488_hw_config_t;

/**
 * @brief Initialize hardware
 * 
 * @param config Hardware configuration parameters
 * @return bool Whether initialization was successful
 */
bool ili9488_hal_init(const ili9488_hw_config_t *config);

/**
 * @brief Perform hardware reset
 */
void ili9488_hal_reset(void);

/**
 * @brief Set data/command pin level
 * 
 * @param level Pin level (true for data, false for command)
 */
void ili9488_hal_dc(bool level);

/**
 * @brief Send command
 * 
 * @param cmd Command byte
 */
void ili9488_hal_write_cmd(uint8_t cmd);

/**
 * @brief Send single data byte
 * 
 * @param data Data byte
 */
void ili9488_hal_write_data(uint8_t data);

/**
 * @brief Send multiple data bytes
 * 
 * @param data Data buffer pointer
 * @param len Data length
 */
void ili9488_hal_write_data_buffer(const uint8_t *data, size_t len);

/**
 * @brief Send data buffer using DMA (asynchronous, non-blocking CPU)
 * 
 * @param data Data pointer
 * @param len Data length
 * @return bool Whether transfer started successfully
 */
bool ili9488_hal_write_data_dma(const uint8_t *data, size_t len);

/**
 * @brief Check if DMA transfer is complete
 * 
 * @return bool Whether transfer is complete
 */
bool ili9488_hal_is_dma_busy(void);

/**
 * @brief Wait for DMA transfer to complete
 */
void ili9488_hal_wait_dma_idle(void);

/**
 * @brief Set backlight state
 * 
 * @param on Backlight state (true for on, false for off)
 */
void ili9488_hal_set_backlight(bool on);

/**
 * @brief Set backlight brightness
 * 
 * @param brightness Brightness level (0-255), 0 for off, 255 for maximum brightness
 */
void ili9488_hal_set_backlight_brightness(uint8_t brightness);

/**
 * @brief Delay milliseconds
 * 
 * @param ms Number of milliseconds
 */
void ili9488_hal_delay_ms(uint32_t ms);

/**
 * @brief Delay microseconds
 * 
 * @param us Number of microseconds
 */
void ili9488_hal_delay_us(uint32_t us);

#endif /* _ILI9488_HAL_H_ */ 