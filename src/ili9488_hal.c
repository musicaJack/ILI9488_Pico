/**
 * @file ili9488_hal.c
 * @brief Hardware Abstraction Layer for ILI9488 LCD module (Raspberry Pi Pico implementation)
 */

#include <stdio.h>
#include <stdlib.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "ili9488_hal.h"

// Global configuration structure for hardware setup
static ili9488_hw_config_t hw_config = {0};

// Function declarations
static inline void ili9488_hal_cs(bool level);
void ili9488_hal_dc(bool level);

// Global DMA channel
static int dma_channel = -1;
static volatile bool dma_busy = false;

// DMA transfer complete interrupt handler
static void dma_complete_handler(void) {
    // Restore CS pin when transfer completes
    ili9488_hal_cs(1);  // CS high (inactive)
    
    // Clear busy flag
    dma_busy = false;
    
    // Clear interrupt flag
    dma_channel_acknowledge_irq0(dma_channel);
}

// Initialize HAL
bool ili9488_hal_init(const ili9488_hw_config_t *config) {
    if (config == NULL) {
        printf("Error: Hardware configuration is NULL\n");
        return false;
    }
    
    // Save configuration to local structure
    hw_config = *config;
    
    // Initialize SPI
    spi_init(hw_config.spi_inst, hw_config.spi_speed_hz);
    
    // Configure SPI GPIO pins
    gpio_set_function(hw_config.pin_sck, GPIO_FUNC_SPI);
    gpio_set_function(hw_config.pin_din, GPIO_FUNC_SPI);
    
    // Configure other control pins as GPIO
    gpio_init(hw_config.pin_cs);
    gpio_init(hw_config.pin_dc);
    gpio_init(hw_config.pin_reset);
    
    gpio_set_dir(hw_config.pin_cs, GPIO_OUT);
    gpio_set_dir(hw_config.pin_dc, GPIO_OUT);
    gpio_set_dir(hw_config.pin_reset, GPIO_OUT);
    
    gpio_put(hw_config.pin_cs, 1);    // Default CS high (inactive)
    gpio_put(hw_config.pin_dc, 1);    // Default DC high (data mode)
    gpio_put(hw_config.pin_reset, 1); // Default RESET high (inactive)
    
    // Configure backlight pin if provided (using PWM for brightness control)
    if (hw_config.pin_bl != -1) {
        // Set up backlight pin for PWM control
        gpio_set_function(hw_config.pin_bl, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(hw_config.pin_bl);
        uint channel = pwm_gpio_to_channel(hw_config.pin_bl);
        
        // Configure PWM
        pwm_config config = pwm_get_default_config();
        pwm_config_set_clkdiv(&config, 4.f); // Divide clock by 4 for slower PWM
        pwm_config_set_wrap(&config, 255);   // 8-bit resolution (0-255)
        pwm_init(slice_num, &config, true);  // Start PWM
        
        // Set initial brightness to max
        pwm_set_chan_level(slice_num, channel, 255);
    }
    
    printf("ILI9488 HAL initialized successfully\n");
    return true;
}

// Set CS line state
static inline void ili9488_hal_cs(bool level) {
    gpio_put(hw_config.pin_cs, level ? 1 : 0);
}

// Set DC line state
void ili9488_hal_dc(bool level) {
    gpio_put(hw_config.pin_dc, level ? 1 : 0);
}

// Reset the display
void ili9488_hal_reset(void) {
    printf("Resetting ILI9488 display...\n");
    
    // Perform hardware reset sequence
    gpio_put(hw_config.pin_reset, 1);
    sleep_ms(10);
    gpio_put(hw_config.pin_reset, 0);
    sleep_ms(10);
    gpio_put(hw_config.pin_reset, 1);
    sleep_ms(150); // Wait for reset to complete
    
    printf("Reset complete\n");
}

// Write command to the display
void ili9488_hal_write_cmd(uint8_t cmd) {
    ili9488_hal_cs(0);          // CS low (active)
    ili9488_hal_dc(0);          // DC low (command mode)
    spi_write_blocking(hw_config.spi_inst, &cmd, 1);
    ili9488_hal_cs(1);          // CS high (inactive)
}

// Write single byte data to the display
void ili9488_hal_write_data(uint8_t data) {
    ili9488_hal_cs(0);          // CS low (active)
    ili9488_hal_dc(1);          // DC high (data mode)
    spi_write_blocking(hw_config.spi_inst, &data, 1);
    ili9488_hal_cs(1);          // CS high (inactive)
}

// Write data buffer to the display
void ili9488_hal_write_data_buffer(const uint8_t *data, size_t len) {
    if (data == NULL || len == 0) {
        return;
    }
    
    ili9488_hal_cs(0);          // CS low (active)
    ili9488_hal_dc(1);          // DC high (data mode)
    
    // Optimize by sending data in chunks
    size_t remaining = len;
    const uint8_t *ptr = data;
    
    while (remaining > 0) {
        // Send data in chunks of up to 4096 bytes
        size_t chunk_size = remaining > 4096 ? 4096 : remaining;
        spi_write_blocking(hw_config.spi_inst, ptr, chunk_size);
        
        ptr += chunk_size;
        remaining -= chunk_size;
    }
    
    ili9488_hal_cs(1);          // CS high (inactive)
}

// Set backlight state
void ili9488_hal_set_backlight(bool on) {
    if (hw_config.pin_bl == -1) {
        // No backlight pin configured
        return;
    }
    
    if (on) {
        // Maximum brightness
        ili9488_hal_set_backlight_brightness(255);
    } else {
        // Turn off backlight
        ili9488_hal_set_backlight_brightness(0);
    }
}

// Set backlight brightness (0-255)
void ili9488_hal_set_backlight_brightness(uint8_t brightness) {
    if (hw_config.pin_bl == -1) {
        // No backlight pin configured
        return;
    }
    
    uint slice_num = pwm_gpio_to_slice_num(hw_config.pin_bl);
    uint channel = pwm_gpio_to_channel(hw_config.pin_bl);
    
    // Set PWM level for brightness control
    pwm_set_chan_level(slice_num, channel, brightness);
}

// Delay function (milliseconds)
void ili9488_hal_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

void ili9488_hal_delay_us(uint32_t us) {
    sleep_us(us);
}

/**
 * @brief Send data buffer using DMA (asynchronous, non-blocking CPU)
 * 
 * @param data Data pointer
 * @param len Data length
 * @return bool Whether transfer started successfully
 */
bool ili9488_hal_write_data_dma(const uint8_t *data, size_t len) {
    // Parameter check
    if (data == NULL || len == 0) {
        return false;
    }
    
    // If DMA channel is not initialized
    if (dma_channel == -1) {
        // Claim a DMA channel
        dma_channel = dma_claim_unused_channel(true);
        if (dma_channel == -1) {
            printf("Error: Failed to claim DMA channel\n");
            return false;
        }
        
        // Configure DMA channel
        dma_channel_config config = dma_channel_get_default_config(dma_channel);
        channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
        channel_config_set_read_increment(&config, true);      // Increment read address
        channel_config_set_write_increment(&config, false);    // Don't increment write address
        channel_config_set_dreq(&config, spi_get_dreq(hw_config.spi_inst, true)); // Triggered by SPI TX FIFO
        
        // Set interrupt handler
        dma_channel_set_irq0_enabled(dma_channel, true);
        irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
        irq_set_enabled(DMA_IRQ_0, true);
        
        // Initialize DMA configuration
        dma_channel_configure(
            dma_channel,
            &config,
            &spi_get_hw(hw_config.spi_inst)->dr,  // Target: SPI data register
            NULL,                                 // Source: Will be set during transfer
            0,                                    // Transfer count: Will be set during transfer
            false                                 // Don't start immediately
        );
    }
    
    // Check if DMA is busy
    if (dma_busy) {
        printf("Error: DMA is busy\n");
        return false;
    }
    
    // Set flag
    dma_busy = true;
    
    // Set DC to data mode
    ili9488_hal_dc(1);  // DC high (data mode)
    
    // Pull CS low to start transfer
    ili9488_hal_cs(0);  // CS low (active)
    
    // Configure and start DMA transfer
    dma_channel_set_read_addr(dma_channel, data, false);
    dma_channel_set_trans_count(dma_channel, len, false);
    dma_channel_start(dma_channel);
    
    return true;
}

/**
 * @brief Check if DMA transfer is complete
 * 
 * @return bool Whether transfer is complete
 */
bool ili9488_hal_is_dma_busy(void) {
    return dma_busy;
}

/**
 * @brief Wait for DMA transfer to complete
 */
void ili9488_hal_wait_dma_idle(void) {
    while (dma_busy) {
        tight_loop_contents();
    }
} 