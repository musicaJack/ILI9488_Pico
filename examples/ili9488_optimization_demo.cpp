/**
 * @file ili9488_modern_optimization_demo.cpp
 * @brief Performance optimization demo for ILI9488 LCD Driver
 * @note Showcases DMA transfers, bulk operations, and template optimizations
 */

#include <cstdio>
#include <memory>
#include <array>
#include <vector>
#include <chrono>
#include <string_view>
#include <algorithm>
#include <cmath>

#include "pico/stdlib.h"
#include "hardware/spi.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Modern C++ ILI9488 driver includes
#include "ili9488_driver.hpp"
#include "pico_ili9488_gfx.hpp"
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

// Performance measurement utility
class PerformanceTimer {
private:
    std::chrono::steady_clock::time_point start_time_;
    
public:
    void start() {
        start_time_ = std::chrono::steady_clock::now();
    }
    
    uint32_t getElapsedMs() const {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
        return duration.count();
    }
    
    uint32_t getElapsedUs() const {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        return duration.count();
    }
};

// Benchmark utility class
class BenchmarkRunner {
private:
    ILI9488Driver& driver_;
    pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx_;
    
public:
    BenchmarkRunner(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx)
        : driver_(driver), gfx_(gfx) {}
    
    // Benchmark: Fill screen performance
    void benchmarkFillScreen() {
        printf("\n=== Fill Screen Benchmark ===\n");
        
        PerformanceTimer timer;
        constexpr int iterations = 10;
        
        // Test different colors
        std::array<uint16_t, 4> colors = {
            rgb565::RED, rgb565::GREEN, rgb565::BLUE, rgb565::WHITE
        };
        
        for (size_t i = 0; i < colors.size(); ++i) {
            timer.start();
            
            for (int iter = 0; iter < iterations; ++iter) {
                driver_.fillScreen(colors[i]);
            }
            
            uint32_t elapsed_ms = timer.getElapsedMs();
            printf("Color %zu: %lu ms (%u iterations), %.2f fps\n", 
                   i, (unsigned long)elapsed_ms, iterations, 
                   (iterations * 1000.0f) / elapsed_ms);
        }
    }
    
    // Benchmark: Pixel drawing performance
    void benchmarkPixelDrawing() {
        printf("\n=== Pixel Drawing Benchmark ===\n");
        
        PerformanceTimer timer;
        constexpr int pixel_count = 10000;
        
        // Random pixel positions
        std::vector<std::pair<uint16_t, uint16_t>> pixels;
        pixels.reserve(pixel_count);
        
        for (int i = 0; i < pixel_count; ++i) {
            pixels.emplace_back(
                rand() % driver_.getWidth(),
                rand() % driver_.getHeight()
            );
        }
        
        // Test individual pixel drawing
        timer.start();
        for (const auto& [x, y] : pixels) {
            driver_.drawPixel(x, y, rgb565::WHITE);
        }
        uint32_t individual_time = timer.getElapsedUs();
        
        printf("Individual pixels: %lu μs (%d pixels), %.2f pixels/ms\n",
               (unsigned long)individual_time, pixel_count, 
               (pixel_count * 1000.0f) / individual_time);
    }
    
    // Benchmark: Bulk rectangle drawing
    void benchmarkRectangles() {
        printf("\n=== Rectangle Drawing Benchmark ===\n");
        
        PerformanceTimer timer;
        constexpr int rect_count = 100;
        
        // Clear screen
        driver_.fillScreen(rgb565::BLACK);
        
        timer.start();
        for (int i = 0; i < rect_count; ++i) {
            uint16_t x = rand() % (driver_.getWidth() - 50);
            uint16_t y = rand() % (driver_.getHeight() - 50);
            uint16_t w = 20 + (rand() % 30);
            uint16_t h = 20 + (rand() % 30);
            uint16_t color = rgb565::from_rgb888(rand() & 0xFF, rand() & 0xFF, rand() & 0xFF);
            
            gfx_.fillRect(x, y, w, h, color);
        }
        uint32_t elapsed_ms = timer.getElapsedMs();
        
        printf("Rectangles: %lu ms (%d rects), %.2f rects/sec\n",
               (unsigned long)elapsed_ms, rect_count, 
               (rect_count * 1000.0f) / elapsed_ms);
    }
    
    // Benchmark: Circle drawing
    void benchmarkCircles() {
        printf("\n=== Circle Drawing Benchmark ===\n");
        
        PerformanceTimer timer;
        constexpr int circle_count = 50;
        
        // Clear screen
        driver_.fillScreen(rgb565::BLACK);
        
        timer.start();
        for (int i = 0; i < circle_count; ++i) {
            uint16_t x = 30 + (rand() % (driver_.getWidth() - 60));
            uint16_t y = 30 + (rand() % (driver_.getHeight() - 60));
            uint16_t r = 5 + (rand() % 25);
            uint16_t color = rgb565::from_rgb888(rand() & 0xFF, rand() & 0xFF, rand() & 0xFF);
            
            gfx_.fillCircle(x, y, r, color);
        }
        uint32_t elapsed_ms = timer.getElapsedMs();
        
        printf("Filled circles: %lu ms (%d circles), %.2f circles/sec\n",
               (unsigned long)elapsed_ms, circle_count, 
               (circle_count * 1000.0f) / elapsed_ms);
    }
    
    // Benchmark: Text rendering
    void benchmarkTextRendering() {
        printf("\n=== Text Rendering Benchmark ===\n");
        
        PerformanceTimer timer;
        constexpr int text_count = 100;
        
        // Clear screen
        driver_.fillScreen(rgb565::BLACK);
        
        // Test text strings
        std::array<std::string_view, 4> test_strings = {
            "Hello World!",
            "ILI9488 Driver",
            "Performance Test",
            "Modern C++ API"
        };
        
        timer.start();
        for (int i = 0; i < text_count; ++i) {
            uint16_t x = rand() % (driver_.getWidth() - 150);
            uint16_t y = rand() % (driver_.getHeight() - 20);
            uint32_t color = rgb888::from_rgb565(rgb565::from_rgb888(
                rand() & 0xFF, rand() & 0xFF, rand() & 0xFF));
            
            const auto& text = test_strings[i % test_strings.size()];
            driver_.drawString(x, y, text, color, rgb888::BLACK);
        }
        uint32_t elapsed_ms = timer.getElapsedMs();
        
        printf("Text rendering: %lu ms (%d strings), %.2f strings/sec\n",
               (unsigned long)elapsed_ms, text_count, 
               (text_count * 1000.0f) / elapsed_ms);
    }
    
    // Benchmark: DMA vs blocking transfers (if DMA available)
    void benchmarkDMATransfers() {
        printf("\n=== DMA Transfer Benchmark ===\n");
        
        // Create test data
        constexpr size_t data_size = 320 * 100 * 3; // 100 lines of RGB666 data
        std::vector<uint8_t> test_data(data_size);
        
        // Fill with pattern
        for (size_t i = 0; i < data_size; i += 3) {
            test_data[i] = (i / 3) & 0xFF;     // R
            test_data[i + 1] = ((i / 3) >> 8) & 0xFF; // G
            test_data[i + 2] = ((i / 3) >> 16) & 0xFF; // B
        }
        
        PerformanceTimer timer;
        constexpr int iterations = 5;
        
        // Test blocking transfers
        timer.start();
        for (int i = 0; i < iterations; ++i) {
            driver_.writePixels(0, i * 20, 319, i * 20 + 19, 
                              reinterpret_cast<const uint16_t*>(test_data.data()),
                              data_size / 6); // Each pixel is 6 bytes in RGB666, but we're passing RGB565
        }
        uint32_t blocking_time = timer.getElapsedMs();
        
        printf("Blocking transfers: %lu ms (%d iterations)\n", (unsigned long)blocking_time, iterations);
        
        // Test DMA transfers (if available)
        if (!driver_.isDMABusy()) {
            timer.start();
            for (int i = 0; i < iterations; ++i) {
                if (driver_.writeDMA(test_data.data(), data_size)) {
                    driver_.waitDMAComplete();
                }
            }
            uint32_t dma_time = timer.getElapsedMs();
            
            printf("DMA transfers: %lu ms (%d iterations)\n", (unsigned long)dma_time, iterations);
            printf("DMA speedup: %.2fx\n", 
                   static_cast<float>(blocking_time) / dma_time);
        } else {
            printf("DMA not available on this configuration\n");
        }
    }
};

// Advanced graphics demonstrations
class AdvancedGraphicsDemo {
private:
    ILI9488Driver& driver_;
    pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx_;
    
public:
    AdvancedGraphicsDemo(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx)
        : driver_(driver), gfx_(gfx) {}
    
    // Animated gradient effect
    void gradientAnimation() {
        printf("\n=== Gradient Animation Demo ===\n");
        
        for (int frame = 0; frame < 60; ++frame) {
            for (uint16_t y = 0; y < driver_.getHeight(); ++y) {
                for (uint16_t x = 0; x < driver_.getWidth(); ++x) {
                    uint8_t r = (x + frame) & 0xFF;
                    uint8_t g = (y + frame) & 0xFF;
                    uint8_t b = ((x + y + frame) / 2) & 0xFF;
                    
                    uint16_t color = rgb565::from_rgb888(r, g, b);
                    driver_.drawPixel(x, y, color);
                }
            }
            sleep_ms(50);
        }
    }
    
    // Mandelbrot fractal rendering
    void mandelbrotFractal() {
        printf("\n=== Mandelbrot Fractal Demo ===\n");
        
        constexpr double zoom = 200.0;
        constexpr double offset_x = -0.5;
        constexpr double offset_y = 0.0;
        constexpr int max_iter = 50;
        
        for (uint16_t py = 0; py < driver_.getHeight(); ++py) {
            for (uint16_t px = 0; px < driver_.getWidth(); ++px) {
                double x0 = (px - driver_.getWidth() / 2.0) / zoom + offset_x;
                double y0 = (py - driver_.getHeight() / 2.0) / zoom + offset_y;
                
                double x = 0.0;
                double y = 0.0;
                int iteration = 0;
                
                while (x*x + y*y <= 4.0 && iteration < max_iter) {
                    double xtemp = x*x - y*y + x0;
                    y = 2*x*y + y0;
                    x = xtemp;
                    iteration++;
                }
                
                // Color based on iteration count
                uint16_t color;
                if (iteration == max_iter) {
                    color = rgb565::BLACK;
                } else {
                    uint8_t r = (iteration * 8) & 0xFF;
                    uint8_t g = (iteration * 16) & 0xFF;
                    uint8_t b = (iteration * 32) & 0xFF;
                    color = rgb565::from_rgb888(r, g, b);
                }
                
                driver_.drawPixel(px, py, color);
            }
            
            // Progress indicator
            if (py % 20 == 0) {
                printf("Rendering: %d%%\n", (py * 100) / driver_.getHeight());
            }
        }
        
        printf("Mandelbrot fractal completed!\n");
    }
    
    // Plasma effect
    void plasmaEffect() {
        printf("\n=== Plasma Effect Demo ===\n");
        
        for (int frame = 0; frame < 120; ++frame) {
            for (uint16_t y = 0; y < driver_.getHeight(); ++y) {
                for (uint16_t x = 0; x < driver_.getWidth(); ++x) {
                    double dx = x - driver_.getWidth() / 2.0;
                    double dy = y - driver_.getHeight() / 2.0;
                    double distance = sqrt(dx*dx + dy*dy);
                    
                    double time = frame * 0.1;
                    double value = sin(distance * 0.02 + time) + 
                                  sin(x * 0.01 + time * 1.5) +
                                  sin(y * 0.01 + time * 2.0);
                    
                    value = (value + 3.0) / 6.0; // Normalize to 0-1
                    
                    uint8_t r = static_cast<uint8_t>(255 * sin(value * M_PI));
                    uint8_t g = static_cast<uint8_t>(255 * sin(value * M_PI + M_PI/3));
                    uint8_t b = static_cast<uint8_t>(255 * sin(value * M_PI + 2*M_PI/3));
                    
                    uint16_t color = rgb565::from_rgb888(r, g, b);
                    driver_.drawPixel(x, y, color);
                }
            }
            sleep_ms(30);
        }
    }
};

int main() {
    stdio_init_all();
    printf("=== ILI9488 Modern C++ Optimization Demo ===\n");
    
    // Initialize driver with RAII
    ILI9488Driver driver(spi0, PIN_DC, PIN_RST, PIN_CS, PIN_SCK, PIN_MOSI, PIN_BL);
    pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver> gfx(driver, 320, 480);
    
    if (!driver.initialize()) {
        printf("Failed to initialize display!\n");
        return -1;
    }
    
    // Set 180 degree rotation to fix upside-down display
    driver.setRotation(Rotation::Portrait_180);  // Fix upside-down display
    
    // Clear the entire screen to prevent display artifacts
    driver.fillScreen(rgb565::BLACK);
    sleep_ms(100);  // Allow display to stabilize
    
    driver.setBacklight(true);
    printf("Display initialized successfully with 180° rotation!\n");
    
    // Performance benchmarks
    BenchmarkRunner benchmark(driver, gfx);
    
    // Run performance tests
    benchmark.benchmarkFillScreen();
    sleep_ms(1000);
    
    benchmark.benchmarkPixelDrawing();
    sleep_ms(1000);
    
    benchmark.benchmarkRectangles();
    sleep_ms(2000);
    
    benchmark.benchmarkCircles();
    sleep_ms(2000);
    
    benchmark.benchmarkTextRendering();
    sleep_ms(2000);
    
    benchmark.benchmarkDMATransfers();
    sleep_ms(1000);
    
    // Advanced graphics demonstrations
    AdvancedGraphicsDemo graphics_demo(driver, gfx);
    
    printf("\nStarting advanced graphics demos...\n");
    printf("Press any key to continue between demos\n");
    
    graphics_demo.gradientAnimation();
    sleep_ms(3000);
    
    graphics_demo.mandelbrotFractal();
    sleep_ms(5000);
    
    graphics_demo.plasmaEffect();
    
    printf("\n=== Optimization Demo Completed! ===\n");
    printf("Key optimizations demonstrated:\n");
    printf("- RAII resource management\n");
    printf("- Template-based graphics engine\n");
    printf("- DMA transfers for bulk operations\n");
    printf("- Efficient color space conversions\n");
    printf("- Hardware-optimized drawing primitives\n");
    printf("- Performance monitoring and benchmarking\n");
    
    while (true) {
        sleep_ms(1000);
    }
    
    return 0;
} 