/**
 * @file ili9488_hal.c
 * @brief Hardware Abstraction Layer implementation for ILI9488 LCD driver
 */

#include <stdio.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "ili9488_hal.h"

// Hardware configuration
static struct {
    bool is_initialized;
    spi_inst_t *spi;
    
    // Pins
    uint8_t pin_din;
    uint8_t pin_sck;
    uint8_t pin_cs;
    uint8_t pin_dc;
    uint8_t pin_reset;
    uint8_t pin_bl;
    
    // PWM related
    uint slice_num;         // Slice number for backlight PWM
    uint channel;           // Channel for backlight PWM
    bool pwm_enabled;       // Whether PWM is enabled
} hw_config = {0};

bool ili9488_hal_init(const ili9488_hw_config_t *config) {
    if (config == NULL) {
        printf("Error: NULL configuration\n");
        return false;
    }
    
    // Save configuration
    hw_config.spi = config->spi_inst;
    hw_config.pin_din = config->pin_din;
    hw_config.pin_sck = config->pin_sck;
    hw_config.pin_cs = config->pin_cs;
    hw_config.pin_dc = config->pin_dc;
    hw_config.pin_reset = config->pin_reset;
    hw_config.pin_bl = config->pin_bl;
    
    printf("Initializing ILI9488 hardware...\n");
    
    // Initialize GPIO pins
    gpio_init(hw_config.pin_dc);
    gpio_init(hw_config.pin_cs);
    gpio_init(hw_config.pin_reset);
    gpio_init(hw_config.pin_bl);
    
    // Set GPIO direction
    gpio_set_dir(hw_config.pin_dc, GPIO_OUT);
    gpio_set_dir(hw_config.pin_cs, GPIO_OUT);
    gpio_set_dir(hw_config.pin_reset, GPIO_OUT);
    gpio_set_dir(hw_config.pin_bl, GPIO_OUT);
    
    // Set initial state
    gpio_put(hw_config.pin_cs, 1);     // Default not selected
    gpio_put(hw_config.pin_dc, 1);     // Default data mode
    gpio_put(hw_config.pin_reset, 1);  // Default not reset
    gpio_put(hw_config.pin_bl, 0);     // Default backlight off
    
    // Initialize SPI
    spi_init(hw_config.spi, config->spi_speed_hz);
    
    // Set SPI format - 8-bit, mode 0, MSB first
    spi_set_format(hw_config.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    // Set SPI pins
    gpio_set_function(hw_config.pin_sck, GPIO_FUNC_SPI);
    gpio_set_function(hw_config.pin_din, GPIO_FUNC_SPI);
    
    // Initialize PWM backlight control
    // Get PWM slice and channel for backlight pin
    hw_config.slice_num = pwm_gpio_to_slice_num(hw_config.pin_bl);
    hw_config.channel = pwm_gpio_to_channel(hw_config.pin_bl);
    
    // Configure GPIO as PWM function
    gpio_set_function(hw_config.pin_bl, GPIO_FUNC_PWM);
    
    // Configure PWM 
    // Use 125MHz / 65535 ≈ 1.9kHz PWM frequency
    pwm_set_wrap(hw_config.slice_num, 65535);
    pwm_set_chan_level(hw_config.slice_num, hw_config.channel, 0);  // Initialize to 0 (off)
    pwm_set_enabled(hw_config.slice_num, true);
    
    hw_config.pwm_enabled = true;
    
    // Perform hardware reset
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
    
    // Hardware reset sequence
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
    
    gpio_put(hw_config.pin_cs, 0);  // Select chip
    gpio_put(hw_config.pin_dc, 0);  // Command mode
    spi_write_blocking(hw_config.spi, &cmd, 1);
    gpio_put(hw_config.pin_cs, 1);  // Deselect chip
}

void ili9488_hal_write_data(uint8_t data) {
    if (!hw_config.is_initialized) {
        return;
    }
    
    gpio_put(hw_config.pin_cs, 0);  // Select chip
    gpio_put(hw_config.pin_dc, 1);  // Data mode
    spi_write_blocking(hw_config.spi, &data, 1);
    gpio_put(hw_config.pin_cs, 1);  // Deselect chip
}

void ili9488_hal_write_data_buffer(const uint8_t *data, size_t len) {
    if (!hw_config.is_initialized || data == NULL || len == 0) {
        return;
    }
    
    gpio_put(hw_config.pin_cs, 0);  // Select chip
    gpio_put(hw_config.pin_dc, 1);  // Data mode
    spi_write_blocking(hw_config.spi, data, len);
    gpio_put(hw_config.pin_cs, 1);  // Deselect chip
}

void ili9488_hal_set_backlight(bool on) {
    if (!hw_config.is_initialized) {
        return;
    }
    
    if (hw_config.pwm_enabled) {
        // If PWM is enabled, set to maximum brightness or off
        pwm_set_chan_level(hw_config.slice_num, hw_config.channel, on ? 65535 : 0);
    } else {
        // Otherwise use GPIO control
        gpio_put(hw_config.pin_bl, on ? 1 : 0);
    }
}

void ili9488_hal_set_backlight_brightness(uint8_t brightness) {
    if (!hw_config.is_initialized) {
        return;
    }
    
    if (hw_config.pwm_enabled) {
        // Map 8-bit brightness value (0-255) to 16-bit PWM range (0-65535)
        uint16_t level = (uint32_t)brightness * 65535 / 255;
        pwm_set_chan_level(hw_config.slice_num, hw_config.channel, level);
    } else {
        // If PWM is not enabled, any non-zero brightness will turn on the backlight
        gpio_put(hw_config.pin_bl, brightness > 0 ? 1 : 0);
    }
}

void ili9488_hal_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

void ili9488_hal_delay_us(uint32_t us) {
    sleep_us(us);
} 