/**
 * @file ili9488_hal.hpp
 * @brief Hardware Abstraction Layer for ILI9488 display driver
 * @author Modern C++ Implementation
 * @note Provides low-level hardware interface for Raspberry Pi Pico
 */

#pragma once

#include <cstdint>
#include <memory>
#include "pico/stdlib.h"
#include "hardware/spi.h"

namespace ili9488 {
namespace hal {

/**
 * @brief Hardware configuration structure
 */
struct HardwareConfig {
    // SPI configuration
    spi_inst_t* spi_inst = spi0;          ///< SPI instance (spi0 or spi1)
    uint32_t spi_speed_hz = 40000000;     ///< SPI speed in Hz (40MHz default)
    
    // GPIO pin assignments
    uint8_t pin_sck = 18;                 ///< SPI clock pin
    uint8_t pin_mosi = 19;                ///< SPI MOSI pin 
    uint8_t pin_miso = 255;               ///< SPI MISO pin (not used, 255 = disabled)
    uint8_t pin_cs = 17;                  ///< Chip select pin
    uint8_t pin_dc = 20;                  ///< Data/Command control pin
    uint8_t pin_rst = 21;                 ///< Reset pin
    uint8_t pin_bl = 22;                  ///< Backlight PWM pin (255 = disabled)
};

/**
 * @brief Hardware abstraction layer for ILI9488
 * @note Implements RAII pattern for resource management
 */
class ILI9488HAL {
public:
    /**
     * @brief Construct HAL with hardware configuration
     * @param config Hardware pin and interface configuration
     */
    explicit ILI9488HAL(const HardwareConfig& config);
    
    /**
     * @brief Destructor - cleanup resources
     */
    ~ILI9488HAL();
    
    // Non-copyable, movable
    ILI9488HAL(const ILI9488HAL&) = delete;
    ILI9488HAL& operator=(const ILI9488HAL&) = delete;
    ILI9488HAL(ILI9488HAL&&) = default;
    ILI9488HAL& operator=(ILI9488HAL&&) = default;
    
    /**
     * @brief Initialize hardware subsystems
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Check if HAL is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return is_initialized_; }
    
    // === Display Control Interface ===
    
    /**
     * @brief Perform hardware reset sequence
     */
    void hardwareReset();
    
    /**
     * @brief Set chip select signal
     * @param active true to activate (low), false to deactivate (high)
     */
    void setChipSelect(bool active);
    
    /**
     * @brief Set data/command control signal
     * @param is_data true for data mode, false for command mode
     */
    void setDataCommand(bool is_data);
    
    // === Data Transfer Interface ===
    
    /**
     * @brief Write command byte to display
     * @param command Command byte to send
     */
    void writeCommand(uint8_t command);
    
    /**
     * @brief Write single data byte
     * @param data Data byte to send
     */
    void writeData(uint8_t data);
    
    /**
     * @brief Write data buffer (blocking)
     * @param data Pointer to data buffer
     * @param length Number of bytes to write
     */
    void writeDataBuffer(const uint8_t* data, size_t length);
    
    /**
     * @brief Write data buffer using DMA (non-blocking)
     * @param data Pointer to data buffer
     * @param length Number of bytes to write
     * @return true if DMA transfer started
     */
    bool writeDataBufferDMA(const uint8_t* data, size_t length);
    
    /**
     * @brief Check if DMA transfer is in progress
     * @return true if DMA is busy
     */
    bool isDMABusy() const { return dma_busy_; }
    
    /**
     * @brief Wait for DMA transfer to complete
     */
    void waitDMAComplete();
    
    // === Backlight Control ===
    
    /**
     * @brief Set backlight brightness
     * @param brightness Brightness level (0-255)
     */
    void setBacklightBrightness(uint8_t brightness);
    
    /**
     * @brief Enable/disable backlight
     * @param enable true to enable, false to disable
     */
    void setBacklight(bool enable);
    
    // === Utility Functions ===
    
    /**
     * @brief Delay in milliseconds
     * @param ms Milliseconds to delay
     */
    void delayMs(uint32_t ms);
    
    /**
     * @brief Delay in microseconds
     * @param us Microseconds to delay
     */
    void delayUs(uint32_t us);
    
    // === Singleton Access ===
    
    /**
     * @brief Get singleton instance
     * @param config Hardware configuration (only used on first call)
     * @return Reference to HAL instance
     */
    static ILI9488HAL& getInstance(const HardwareConfig& config = {});
    
    /**
     * @brief Check if instance exists
     * @return true if instance exists
     */
    static bool hasInstance() { return instance_ != nullptr; }

private:
    // Hardware configuration
    HardwareConfig config_;
    
    // State tracking
    bool is_initialized_;
    
    // DMA management
    int dma_channel_;
    volatile bool dma_busy_;
    
    // Singleton instance
    static ILI9488HAL* instance_;
    
    // === Private Initialization Methods ===
    
    /**
     * @brief Initialize SPI interface
     * @return true if successful
     */
    bool initializeSPI();
    
    /**
     * @brief Initialize GPIO pins
     * @return true if successful
     */
    bool initializeGPIO();
    
    /**
     * @brief Initialize PWM for backlight
     * @return true if successful
     */
    bool initializePWM();
    
    /**
     * @brief Initialize DMA (optional)
     * @return true if successful
     */
    bool initializeDMA();
    
    /**
     * @brief Cleanup hardware resources
     */
    void cleanup();
    
    /**
     * @brief DMA interrupt handler
     */
    static void dmaInterruptHandler();
};

} // namespace hal
} // namespace ili9488 