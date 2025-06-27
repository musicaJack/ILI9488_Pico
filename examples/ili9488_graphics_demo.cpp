/**
 * @file ili9488_modern_graphics_demo.cpp
 * @brief Advanced graphics demo for ILI9488 LCD Driver
 * @note Showcases template graphics engine, effects, and modern C++ features
 */

#include <cstdio>
#include <memory>
#include <array>
#include <vector>
#include <cmath>
#include <string_view>
#include <functional>

#include "pico/stdlib.h"
#include "hardware/spi.h"

// Modern C++ ILI9488 driver includes
#include "ili9488_driver.hpp"
#include "pico_ili9488_gfx.hpp"
#include "ili9488_colors.hpp"
#include "ili9488_font.hpp"

// 统一引脚配置
#include "pin_config.hpp"

using namespace ili9488;
using namespace ili9488_colors;

// Demo scene interface
class DemoScene {
public:
    virtual ~DemoScene() = default;
    virtual void render(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx) = 0;
    virtual std::string_view getName() const = 0;
    virtual uint32_t getDurationMs() const { return 5000; }
};

// Geometric patterns demo
class GeometricPatternsDemo : public DemoScene {
public:
    void render(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx) override {
        printf("Rendering geometric patterns...\n");
        
        gfx.clearScreenFast(rgb565::BLACK);
        
        const uint16_t center_x = driver.getWidth() / 2;
        const uint16_t center_y = driver.getHeight() / 2;
        
        // Concentric circles with color gradients
        for (int r = 10; r < 150; r += 15) {
            uint8_t hue = (r * 2) % 256;
            uint16_t color = hsvToRgb565(hue, 255, 255);
            gfx.drawCircle(center_x, center_y, r, color);
        }
        
        // Radial lines
        for (int angle = 0; angle < 360; angle += 15) {
            float rad = angle * M_PI / 180.0f;
            int x1 = center_x + 50 * cos(rad);
            int y1 = center_y + 50 * sin(rad);
            int x2 = center_x + 140 * cos(rad);
            int y2 = center_y + 140 * sin(rad);
            
            uint16_t color = hsvToRgb565(angle, 255, 200);
            gfx.drawLine(x1, y1, x2, y2, color);
        }
        
        // Corner patterns
        drawCornerPattern(gfx, 0, 0, 100, rgb565::CYAN);
        drawCornerPattern(gfx, driver.getWidth() - 100, 0, 100, rgb565::MAGENTA);
        drawCornerPattern(gfx, 0, driver.getHeight() - 100, 100, rgb565::YELLOW);
        drawCornerPattern(gfx, driver.getWidth() - 100, driver.getHeight() - 100, 100, rgb565::GREEN);
    }
    
    std::string_view getName() const override { return "Geometric Patterns"; }
    
private:
    void drawCornerPattern(pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx, 
                          int x, int y, int size, uint16_t base_color) {
        for (int i = 0; i < size; i += 10) {
            uint8_t alpha = 255 - (i * 255 / size);
            uint16_t color = blendColors(base_color, rgb565::BLACK, alpha);
            
            gfx.drawRect(x + i/2, y + i/2, size - i, size - i, color);
        }
    }
    
    uint16_t hsvToRgb565(uint8_t h, uint8_t s, uint8_t v) {
        uint8_t r, g, b;
        
        if (s == 0) {
            r = g = b = v;
        } else {
            uint8_t region = h / 43;
            uint8_t remainder = (h - (region * 43)) * 6;
            
            uint8_t p = (v * (255 - s)) >> 8;
            uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
            uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
            
            switch (region) {
                case 0: r = v; g = t; b = p; break;
                case 1: r = q; g = v; b = p; break;
                case 2: r = p; g = v; b = t; break;
                case 3: r = p; g = q; b = v; break;
                case 4: r = t; g = p; b = v; break;
                default: r = v; g = p; b = q; break;
            }
        }
        
        return rgb565::from_rgb888(r, g, b);
    }
    
    uint16_t blendColors(uint16_t color1, uint16_t color2, uint8_t alpha) {
        uint8_t r1 = (color1 >> 11) & 0x1F;
        uint8_t g1 = (color1 >> 5) & 0x3F;
        uint8_t b1 = color1 & 0x1F;
        
        uint8_t r2 = (color2 >> 11) & 0x1F;
        uint8_t g2 = (color2 >> 5) & 0x3F;
        uint8_t b2 = color2 & 0x1F;
        
        uint8_t r = (r1 * alpha + r2 * (255 - alpha)) / 255;
        uint8_t g = (g1 * alpha + g2 * (255 - alpha)) / 255;
        uint8_t b = (b1 * alpha + b2 * (255 - alpha)) / 255;
        
        return (r << 11) | (g << 5) | b;
    }
};

// Animated sprites demo
class AnimatedSpritesDemo : public DemoScene {
private:
    struct Sprite {
        float x, y;
        float vx, vy;
        uint16_t color;
        uint8_t size;
        uint8_t type;
    };
    
    std::vector<Sprite> sprites_;
    
public:
    AnimatedSpritesDemo() {
        // Initialize sprites
        sprites_.reserve(20);
        for (int i = 0; i < 20; ++i) {
            Sprite sprite{
                .x = static_cast<float>(rand() % 320),
                .y = static_cast<float>(rand() % 480),
                .vx = (rand() % 40 - 20) / 10.0f,
                .vy = (rand() % 40 - 20) / 10.0f,
                .color = static_cast<uint16_t>(rand() & 0xFFFF),
                .size = static_cast<uint8_t>(5 + rand() % 15),
                .type = static_cast<uint8_t>(rand() % 3)
            };
            sprites_.push_back(sprite);
        }
    }
    
    void render(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx) override {
        printf("Rendering animated sprites...\n");
        
        for (int frame = 0; frame < 300; ++frame) {
            gfx.clearScreenFast(rgb565::BLACK);
            
            // Update and draw sprites
            for (auto& sprite : sprites_) {
                // Update position
                sprite.x += sprite.vx;
                sprite.y += sprite.vy;
                
                // Bounce off walls
                if (sprite.x <= 0 || sprite.x >= driver.getWidth() - sprite.size) {
                    sprite.vx = -sprite.vx;
                    sprite.x = std::clamp(sprite.x, 0.0f, static_cast<float>(driver.getWidth() - sprite.size));
                }
                if (sprite.y <= 0 || sprite.y >= driver.getHeight() - sprite.size) {
                    sprite.vy = -sprite.vy;
                    sprite.y = std::clamp(sprite.y, 0.0f, static_cast<float>(driver.getHeight() - sprite.size));
                }
                
                // Draw sprite based on type
                switch (sprite.type) {
                    case 0: // Circle
                        gfx.fillCircle(static_cast<int>(sprite.x), static_cast<int>(sprite.y), 
                                     sprite.size, sprite.color);
                        break;
                    case 1: // Rectangle
                        gfx.fillRect(static_cast<int>(sprite.x), static_cast<int>(sprite.y), 
                                   sprite.size, sprite.size, sprite.color);
                        break;
                    case 2: // Triangle
                        drawTriangleSprite(gfx, static_cast<int>(sprite.x), static_cast<int>(sprite.y), 
                                         sprite.size, sprite.color);
                        break;
                }
            }
            
            // Draw frame counter
            char frame_text[32];
            snprintf(frame_text, sizeof(frame_text), "Frame: %d", frame);
            driver.drawString(10, 10, frame_text, rgb888::WHITE, rgb888::BLACK);
            
            sleep_ms(33); // ~30 FPS
        }
    }
    
    std::string_view getName() const override { return "Animated Sprites"; }
    uint32_t getDurationMs() const override { return 10000; }
    
private:
    void drawTriangleSprite(pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx, 
                           int x, int y, int size, uint16_t color) {
        gfx.fillTriangle(x, y + size, 
                        x + size, y + size, 
                        x + size/2, y, color);
    }
};

// Fractal explorer demo
class FractalExplorerDemo : public DemoScene {
public:
    void render(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& /* gfx */) override {
        printf("Rendering fractal explorer...\n");
        
        // Julia Set parameters
        const double zoom_start = 100.0;
        const double zoom_end = 1000.0;
        const int frames = 60;
        
        for (int frame = 0; frame < frames; ++frame) {
            double zoom = zoom_start + (zoom_end - zoom_start) * frame / frames;
            double offset_x = -0.7;
            double offset_y = 0.0;
            
            renderJuliaSet(driver, offset_x, offset_y, zoom, -0.8, 0.156);
            
            // Draw zoom level
            char zoom_text[32];
            snprintf(zoom_text, sizeof(zoom_text), "Zoom: %.0fx", zoom);
            driver.drawString(10, driver.getHeight() - 20, zoom_text, rgb888::YELLOW, rgb888::BLACK);
            
            sleep_ms(100);
        }
    }
    
    std::string_view getName() const override { return "Fractal Explorer"; }
    uint32_t getDurationMs() const override { return 8000; }
    
private:
    void renderJuliaSet(ILI9488Driver& driver, double offset_x, double offset_y, 
                       double zoom, double cx, double cy) {
        const int max_iter = 50;
        const uint16_t width = driver.getWidth();
        const uint16_t height = driver.getHeight();
        
        for (uint16_t py = 0; py < height; py += 2) { // Skip every other line for speed
            for (uint16_t px = 0; px < width; px += 2) { // Skip every other pixel
                double x = (px - width / 2.0) / zoom + offset_x;
                double y = (py - height / 2.0) / zoom + offset_y;
                
                int iteration = 0;
                while (x*x + y*y <= 4.0 && iteration < max_iter) {
                    double xtemp = x*x - y*y + cx;
                    y = 2*x*y + cy;
                    x = xtemp;
                    iteration++;
                }
                
                uint16_t color;
                if (iteration == max_iter) {
                    color = rgb565::BLACK;
                } else {
                    uint8_t r = (iteration * 8) & 0xFF;
                    uint8_t g = (iteration * 16) & 0xFF;
                    uint8_t b = (iteration * 32) & 0xFF;
                    color = rgb565::from_rgb888(r, g, b);
                }
                
                // Draw 2x2 block for speed
                driver.drawPixel(px, py, color);
                if (px + 1 < width) driver.drawPixel(px + 1, py, color);
                if (py + 1 < height) driver.drawPixel(px, py + 1, color);
                if (px + 1 < width && py + 1 < height) driver.drawPixel(px + 1, py + 1, color);
            }
        }
    }
};

// Interactive dashboard demo
class InteractiveDashboardDemo : public DemoScene {
public:
    void render(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx) override {
        printf("Rendering interactive dashboard...\n");
        
        for (int frame = 0; frame < 120; ++frame) {
            gfx.clearScreenFast(rgb565::NAVY);
            
            // Title bar
            gfx.fillRect(0, 0, driver.getWidth(), 30, rgb565::DARKBLUE);
            driver.drawString(10, 8, "System Dashboard", rgb888::WHITE, rgb888::DARKBLUE);
            
            // CPU usage gauge
            drawGauge(driver, gfx, 60, 80, 50, "CPU", (50 + 30 * sin(frame * 0.1)), rgb565::RED);
            
            // Memory usage gauge
            drawGauge(driver, gfx, 200, 80, 50, "RAM", (60 + 20 * cos(frame * 0.15)), rgb565::GREEN);
            
            // Temperature gauge
            drawGauge(driver, gfx, 60, 200, 50, "TEMP", (40 + 15 * sin(frame * 0.08)), rgb565::ORANGE);
            
            // Network activity
            drawNetworkActivity(driver, gfx, 200, 200, frame);
            
            // Status bars
            drawStatusBars(driver, gfx, 10, 320);
            
            // Real-time graph
            drawRealtimeGraph(driver, gfx, 10, 380, frame);
            
            sleep_ms(100);
        }
    }
    
    std::string_view getName() const override { return "Interactive Dashboard"; }
    uint32_t getDurationMs() const override { return 12000; }
    
private:
    void drawGauge(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx,
                   int cx, int cy, int radius, const char* label, float percentage, uint16_t color) {
        // Gauge background
        gfx.drawCircle(cx, cy, radius, rgb565::DARKGRAY);
        gfx.drawCircle(cx, cy, radius - 5, rgb565::DARKGRAY);
        
        // Gauge fill
        float angle = (percentage / 100.0f) * 270.0f - 135.0f; // -135 to +135 degrees
        float rad = angle * M_PI / 180.0f;
        
        int x1 = cx + (radius - 10) * cos(rad);
        int y1 = cy + (radius - 10) * sin(rad);
        
        gfx.drawLine(cx, cy, x1, y1, color);
        
        // Label and value
        driver.drawString(cx - 20, cy + radius + 10, label, rgb888::WHITE, rgb888::NAVY);
        
        char value_text[16];
        snprintf(value_text, sizeof(value_text), "%.0f%%", percentage);
        driver.drawString(cx - 15, cy + radius + 25, value_text, rgb888::from_rgb565(color), rgb888::NAVY);
    }
    
    void drawNetworkActivity(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx,
                           int x, int y, int frame) {
        driver.drawString(x, y - 15, "Network", rgb888::WHITE, rgb888::NAVY);
        
        // Download indicator
        int down_activity = 20 + 15 * sin(frame * 0.2);
        gfx.fillRect(x, y, 60, down_activity, rgb565::GREEN);
        gfx.drawRect(x, y, 60, 50, rgb565::WHITE);
        driver.drawString(x + 5, y + 55, "DOWN", rgb888::GREEN, rgb888::NAVY);
        
        // Upload indicator  
        int up_activity = 15 + 10 * cos(frame * 0.25);
        gfx.fillRect(x + 80, y, 60, up_activity, rgb565::RED);
        gfx.drawRect(x + 80, y, 60, 50, rgb565::WHITE);
        driver.drawString(x + 85, y + 55, "UP", rgb888::RED, rgb888::NAVY);
    }
    
    void drawStatusBars(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx,
                       int x, int y) {
        const char* labels[] = {"Power", "Signal", "Battery"};
        uint16_t colors[] = {rgb565::YELLOW, rgb565::CYAN, rgb565::GREEN};
        float values[] = {85.0f, 70.0f, 92.0f};
        
        for (int i = 0; i < 3; ++i) {
            int bar_y = y + i * 25;
            
            driver.drawString(x, bar_y, labels[i], rgb888::WHITE, rgb888::NAVY);
            
            // Background bar
            gfx.fillRect(x + 60, bar_y, 200, 15, rgb565::DARKGRAY);
            
            // Value bar
            int bar_width = static_cast<int>(200 * values[i] / 100.0f);
            gfx.fillRect(x + 60, bar_y, bar_width, 15, colors[i]);
            
            // Percentage text
            char percent_text[8];
            snprintf(percent_text, sizeof(percent_text), "%.0f%%", values[i]);
            driver.drawString(x + 270, bar_y, percent_text, rgb888::WHITE, rgb888::NAVY);
        }
    }
    
    void drawRealtimeGraph(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx,
                          int x, int y, int frame) {
        driver.drawString(x, y - 15, "Performance Graph", rgb888::WHITE, rgb888::NAVY);
        
        // Graph background
        gfx.fillRect(x, y, 300, 80, rgb565::BLACK);
        gfx.drawRect(x, y, 300, 80, rgb565::WHITE);
        
        // Draw graph data
        for (int i = 0; i < 299; ++i) {
            float value1 = 40 + 20 * sin((frame - i) * 0.1);
            float value2 = 30 + 15 * cos((frame - i) * 0.15);
            
            int y1 = y + 80 - static_cast<int>(value1 * 80 / 100);
            int y2 = y + 80 - static_cast<int>(value2 * 80 / 100);
            
            gfx.drawPixel(x + i, y1, rgb565::RED);
            gfx.drawPixel(x + i, y2, rgb565::GREEN);
        }
    }
};

// Demo manager class
class DemoManager {
private:
    std::vector<std::unique_ptr<DemoScene>> scenes_;
    size_t current_scene_ = 0;
    
public:
    DemoManager() {
        scenes_.push_back(std::make_unique<GeometricPatternsDemo>());
        scenes_.push_back(std::make_unique<AnimatedSpritesDemo>());
        scenes_.push_back(std::make_unique<FractalExplorerDemo>());
        scenes_.push_back(std::make_unique<InteractiveDashboardDemo>());
    }
    
    void runDemo(ILI9488Driver& driver, pico_ili9488_gfx::PicoILI9488GFX<ILI9488Driver>& gfx) {
        printf("Starting graphics demo with %zu scenes...\n", scenes_.size());
        
        for (const auto& scene : scenes_) {
            printf("\n--- %.*s ---\n", 
                   static_cast<int>(scene->getName().length()), 
                   scene->getName().data());
            
            scene->render(driver, gfx);
            sleep_ms(2000); // Pause between scenes
        }
        
        printf("\nDemo completed!\n");
    }
};

int main() {
    stdio_init_all();
    printf("=== ILI9488 Modern C++ Graphics Demo ===\n");
    
    // Initialize driver with RAII
    ILI9488Driver driver(ILI9488_GET_SPI_CONFIG());
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
    
    // Create and run demo manager
    DemoManager demo_manager;
    demo_manager.runDemo(driver, gfx);
    
    printf("\n=== Graphics Demo Features Showcased ===\n");
    printf("- Template-based graphics engine\n");
    printf("- Advanced drawing primitives\n");
    printf("- Color space manipulations\n");
    printf("- Real-time animations\n");
    printf("- Fractal mathematics\n");
    printf("- Interactive UI components\n");
    printf("- Performance optimizations\n");
    printf("- Modern C++ patterns (RAII, smart pointers, etc.)\n");
    
    while (true) {
        sleep_ms(1000);
    }
    
    return 0;
} 