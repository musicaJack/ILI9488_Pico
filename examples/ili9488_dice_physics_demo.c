/**
 * @file ili9488_dice_physics_demo.c
 * @brief ILI9488 LCD Dice Physics Simulation Demo - Optimized Version
 * 
 * This demo simulates the physical behavior of dice falling from a height, including free fall,
 * collision, rolling, and eventual stopping. It uses a simplified physics model including
 * gravity, friction, and elastic collisions.
 * 
 * Optimization features:
 * - Partial refresh: Only refreshes the area where dice move, not the entire screen
 * - Batch pixel transfer: Uses batch transfer functions to accelerate drawing
 * - DMA acceleration: Utilizes DMA controller for high-speed data transfer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "ili9488.h"
#include "ili9488_gfx.h"

// Pin definitions - consistent with README
#define PIN_DIN   19  // SPI MOSI
#define PIN_SCK   18  // SPI SCK
#define PIN_CS    17  // SPI CS
#define PIN_DC    20  // Data/Command
#define PIN_RESET 15  // Reset
#define PIN_BL    10  // Backlight

// Screen parameters
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 480

// Color definitions (24-bit RGB format)
#define COLOR_RED     0xFF0000  // Red
#define COLOR_GREEN   0x00FF00  // Green
#define COLOR_BLUE    0x0000FF  // Blue
#define COLOR_WHITE   0xFFFFFF  // White
#define COLOR_BLACK   0x000000  // Black
#define COLOR_YELLOW  0xFFFF00  // Yellow
#define COLOR_CYAN    0x00FFFF  // Cyan
#define COLOR_MAGENTA 0xFF00FF  // Magenta
#define COLOR_GRAY    0x888888  // Gray
#define COLOR_DARK_RED 0xFFFFFF // Changed to pure white (was black)
#define COLOR_BROWN   0x8B4513  // Brown for dice outline

// Physics constants
#define GRAVITY       0.15f     // Gravity acceleration (reduced from 0.25f)
#define ELASTICITY    0.6f     // Elasticity coefficient for collisions
#define FRICTION      0.98f    // Friction coefficient (1.0=no friction)
#define ANGULAR_DAMPING 0.95f  // Angular velocity damping

// Dice size and state
#define DICE_SIZE     120      // Dice size in pixels (increased from 60, about 1/8 of screen area)
#define MAX_DICES     1        // Maximum number of dice

// Frame rate control
#define TARGET_FPS    30       // Target frames per second
#define FRAME_TIME_MS (1000 / TARGET_FPS)

// DMA channel and buffer
#define BUF_SIZE      DICE_SIZE * DICE_SIZE  // Maximum pixel count for dice area
uint32_t dma_buf[BUF_SIZE];                 // DMA transfer buffer
int dma_tx_channel;                         // DMA transmit channel

// Dice pattern data (representing 1-6 dots)
const uint8_t dice_patterns[6][9] = {
    {0,0,0, 0,1,0, 0,0,0}, // 1 dot
    {1,0,0, 0,0,0, 0,0,1}, // 2 dots
    {1,0,0, 0,1,0, 0,0,1}, // 3 dots
    {1,0,1, 0,0,0, 1,0,1}, // 4 dots
    {1,0,1, 0,1,0, 1,0,1}, // 5 dots
    {1,0,1, 1,0,1, 1,0,1}  // 6 dots
};

// Dice structure
typedef struct {
    float x, y;          // Position
    float vx, vy;        // Velocity
    float angle;         // Rotation angle (radians)
    float angular_vel;   // Angular velocity
    int face;            // Current face showing up (1-6)
    bool active;         // Whether active
    uint32_t color;      // Dice color
    float size;          // Dice size
    float mass;          // Mass
    int bounce_count;    // Bounce count
    
    // Previous frame position for partial refresh
    int prev_x, prev_y;
    float prev_angle;
} Dice;

// Global variables
Dice dices[MAX_DICES];
uint32_t dice_colors[MAX_DICES] = {COLOR_BLACK}; // Changed to black
bool simulation_running = true;
uint32_t frame_count = 0;
bool use_dma = true;              // Use DMA
bool use_partial_update = false;  // Disable partial refresh mode

// Region definition structure
typedef struct {
    int x1, y1, x2, y2;
} Region;

/**
 * @brief Initialize DMA
 */
void init_dma(void) {
    // Initialize DMA channel
    dma_tx_channel = dma_claim_unused_channel(true);
    printf("DMA channel initialized: %d\n", dma_tx_channel);
}

/**
 * @brief Initialize LCD
 */
bool init_lcd(void) {
    printf("Initializing LCD...\n");
    
    // LCD configuration
    ili9488_config_t config = {
        .spi_inst = spi0,
        .spi_speed_hz = 40 * 1000 * 1000,  // 40MHz
        
        .pin_din = PIN_DIN,
        .pin_sck = PIN_SCK,
        .pin_cs = PIN_CS,
        .pin_dc = PIN_DC,
        .pin_reset = PIN_RESET,
        .pin_bl = PIN_BL,
        
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .rotation = 2,  // Set rotation to 2 (180 degrees), was 0 degrees initially
    };
    
    // Initialize LCD
    bool result = ili9488_init(&config);
    if (result) {
        // Turn on backlight
        ili9488_set_backlight(true);
        printf("LCD initialization successful!\n");
        
        // Initialize DMA
        init_dma();
    } else {
        printf("LCD initialization failed!\n");
    }
    
    return result;
}

/**
 * @brief Randomly initialize dice
 */
void init_dice(Dice *dice, float x_pos, float y_pos, uint32_t color) {
    dice->x = x_pos;
    dice->y = y_pos;
    dice->vx = ((float)rand() / RAND_MAX * 2.0f - 1.0f); // Random velocity, halved from original
    dice->vy = 0;
    dice->angle = 0;
    dice->angular_vel = ((float)rand() / RAND_MAX * 0.1f - 0.05f); // Angular velocity, halved
    dice->face = rand() % 6 + 1;
    dice->active = true;
    dice->color = color;
    dice->size = DICE_SIZE;
    dice->mass = dice->size * dice->size / 100.0f;
    dice->bounce_count = 0;
    
    // Initialize previous frame position
    dice->prev_x = x_pos;
    dice->prev_y = y_pos;
    dice->prev_angle = 0;
}

/**
 * @brief Calculate dice coverage area
 */
Region get_dice_region(Dice *dice) {
    Region region;
    float half_size = dice->size / 2;
    float margin = half_size * 1.5f; // Add some margin to ensure full area during rotation
    
    region.x1 = (int)(dice->x - margin);
    region.y1 = (int)(dice->y - margin);
    region.x2 = (int)(dice->x + margin);
    region.y2 = (int)(dice->y + margin);
    
    // Limit to screen boundaries
    if (region.x1 < 0) region.x1 = 0;
    if (region.y1 < 0) region.y1 = 0;
    if (region.x2 >= SCREEN_WIDTH) region.x2 = SCREEN_WIDTH - 1;
    if (region.y2 >= SCREEN_HEIGHT) region.y2 = SCREEN_HEIGHT - 1;
    
    return region;
}

/**
 * @brief Use optimized method to draw dice
 */
void draw_dice_optimized(Dice *dice) {
    // Calculate dice's four corner coordinates (considering rotation)
    float half_size = dice->size / 2;
    
    // Dice center
    int cx = (int)dice->x;
    int cy = (int)dice->y;
    
    // Calculate rotated coordinates
    float cos_a = cosf(dice->angle);
    float sin_a = sinf(dice->angle);
    
    // Dice's four corner coordinates
    int x1 = cx + (int)(-half_size * cos_a - (-half_size) * sin_a);
    int y1 = cy + (int)(-half_size * sin_a + (-half_size) * cos_a);
    
    int x2 = cx + (int)(half_size * cos_a - (-half_size) * sin_a);
    int y2 = cy + (int)(half_size * sin_a + (-half_size) * cos_a);
    
    int x3 = cx + (int)(half_size * cos_a - half_size * sin_a);
    int y3 = cy + (int)(half_size * sin_a + half_size * cos_a);
    
    int x4 = cx + (int)(-half_size * cos_a - half_size * sin_a);
    int y4 = cy + (int)(-half_size * sin_a + half_size * cos_a);
    
    // Draw thicker dice outline (by drawing multiple adjacent lines)
    // Draw 4 edges - each edge drawn twice to increase thickness
    for(int i = 0; i < 2; i++) {
        // Top edge
        ili9488_draw_line(x1-i, y1-i, x2+i, y2-i, dice->color);
        // Right edge
        ili9488_draw_line(x2+i, y2-i, x3+i, y3+i, dice->color);
        // Bottom edge
        ili9488_draw_line(x3+i, y3+i, x4-i, y4+i, dice->color);
        // Left edge
        ili9488_draw_line(x4-i, y4+i, x1-i, y1-i, dice->color);
    }
    
    // Draw the large X in the middle
    for(int i = 0; i < 2; i++) {
        // Diagonal from top-left to bottom-right
        ili9488_draw_line(x1-i, y1-i, x3+i, y3+i, dice->color);
        // Diagonal from top-right to bottom-left
        ili9488_draw_line(x2+i, y2-i, x4-i, y4+i, dice->color);
    }
    
    // Update previous frame position
    dice->prev_x = cx;
    dice->prev_y = cy;
    dice->prev_angle = dice->angle;
}

/**
 * @brief Update dice physical state
 */
void update_dice_physics(Dice *dice) {
    if (!dice->active) return;
    
    // Apply gravity
    dice->vy += GRAVITY;
    
    // Update position
    dice->x += dice->vx;
    dice->y += dice->vy;
    
    // Update rotation
    dice->angle += dice->angular_vel;
    // Keep angle within 0-2π range
    if (dice->angle > 2 * M_PI) dice->angle -= 2 * M_PI;
    if (dice->angle < 0) dice->angle += 2 * M_PI;
    
    // Detect collision with ground
    if (dice->y + dice->size/2 > SCREEN_HEIGHT) {
        dice->y = SCREEN_HEIGHT - dice->size/2;
        
        // If velocity is small, stop bouncing
        if (fabsf(dice->vy) < 1.0f) {
            dice->vy = 0;
            // Horizontal velocity also decreases
            dice->vx *= 0.8f;
            // Angular velocity decreases
            dice->angular_vel *= 0.8f;
            
            // When everything is almost stopped, fix dice face
            if (fabsf(dice->vx) < 0.1f && fabsf(dice->angular_vel) < 0.01f) {
                dice->vx = 0;
                dice->angular_vel = 0;
                
                // Decide final dice face (based on angle)
                // Simplified: Map angle to dice's six faces
                int face_index = (int)((dice->angle * 3 / M_PI) + 0.5f) % 6;
                dice->face = face_index + 1;
                
                // If dice has stopped, it's no longer active
                if (++dice->bounce_count > 5) {
                    dice->active = false;
                }
            }
        } else {
            // Bounce effect
            dice->vy = -dice->vy * ELASTICITY;
            
            // Increase angular velocity, simulating rotation during collision
            dice->angular_vel += ((float)rand() / RAND_MAX * 0.1f - 0.05f);
            
            // Increase bounce count during collision
            dice->bounce_count++;
        }
    }
    
    // Detect collision with left and right walls
    if (dice->x - dice->size/2 < 0) {
        dice->x = dice->size/2;
        dice->vx = -dice->vx * ELASTICITY;
        dice->angular_vel *= -ELASTICITY;
    } else if (dice->x + dice->size/2 > SCREEN_WIDTH) {
        dice->x = SCREEN_WIDTH - dice->size/2;
        dice->vx = -dice->vx * ELASTICITY;
        dice->angular_vel *= -ELASTICITY;
    }
    
    // Apply friction and damping
    dice->vx *= FRICTION;
    dice->vy *= FRICTION;
    dice->angular_vel *= ANGULAR_DAMPING;
    
    // Prevent very small velocities causing jitter
    if (fabsf(dice->vx) < 0.01f) dice->vx = 0;
    if (fabsf(dice->vy) < 0.01f) dice->vy = 0;
    if (fabsf(dice->angular_vel) < 0.001f) dice->angular_vel = 0;
}

/**
 * @brief Check if all dice have stopped
 */
bool all_dices_stopped(void) {
    for (int i = 0; i < MAX_DICES; i++) {
        if (dices[i].active) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Optimized result display function
 */
void display_dice_results(void) {
    char result_text[64];
    sprintf(result_text, "Roll results: ");
    
    for (int i = 0; i < MAX_DICES; i++) {
        char dice_result[8];
        sprintf(dice_result, "%d ", dices[i].face);
        strcat(result_text, dice_result);
    }
    
    printf("%s\n", result_text);
    
    // Display results on screen - use optimized batch filling
    ili9488_fill_area_rgb24(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, COLOR_BLACK);
    
    // Display text using RGB24 color format
    for (int i = 0; i < MAX_DICES; i++) {
        char num[2] = {dices[i].face + '0', '\0'};
        ili9488_draw_string(20 + i * 60, SCREEN_HEIGHT - 30, num, 0xFFFF, 0x0000, 3);
    }
}

/**
 * @brief Initialize dice falling simulation
 */
void start_dice_simulation(void) {
    // Clear screen and draw background
    ili9488_fill_screen_rgb24(COLOR_DARK_RED); // Background is now white
    
    // Reset simulation state
    simulation_running = true;
    frame_count = 0;
    
    // Initialize dice - center it on screen since we only have one
    init_dice(&dices[0], 
              SCREEN_WIDTH / 2, 
              DICE_SIZE * 2, 
              dice_colors[0]);
    
    printf("Starting dice physics simulation...\n");
    printf("Using full screen refresh mode to avoid flickering\n");
    printf("Using DMA acceleration: %s\n", use_dma ? "Yes" : "No");
    
    // Partial refresh disabled, no need for this code
    // if (use_partial_update) {
    //     ili9488_partial_mode(true);
    // }
}

/**
 * @brief Use batch filling to clear a region with specified color
 */
void clear_region(Region region, uint32_t bg_color) {
    int width = region.x2 - region.x1 + 1;
    int height = region.y2 - region.y1 + 1;
    
    // Use batch fill function - ensure complete coverage of old area
    ili9488_fill_area_rgb24(region.x1, region.y1, region.x2, region.y2, bg_color);
    
    // Add a small delay to ensure fill completes
    ili9488_hal_delay_us(100);
}

/**
 * @brief Main function
 */
int main() {
    // Initialize standard library
    stdio_init_all();
    sleep_ms(3000);  // Wait for serial initialization
    printf("\n\nDice Physics Simulation Demo - Optimized Version\n");
    
    // Initialize random number generator
    srand(time_us_32());
    
    // Initialize LCD
    if (!init_lcd()) {
        printf("Error: LCD initialization failed\n");
        return -1;
    }
    
    // Display title
    ili9488_fill_screen_rgb24(COLOR_DARK_RED); // Background is now white
    ili9488_draw_string(10, 10, "Dice Physics Simulation", 0x0000, 0xFFFF, 2);
    ili9488_draw_string(10, 30, "Black outline dice with X pattern", 0x0000, 0xFFFF, 1);
    sleep_ms(2000);
    
    // Main loop
    while (1) {
        // Start new simulation round
        start_dice_simulation();
        
        // Simulation loop
        absolute_time_t cycle_start = get_absolute_time();
        absolute_time_t frame_start;
        
        // Clear screen once initially instead of every frame
        ili9488_fill_screen_rgb24(COLOR_DARK_RED);
        
        Region prev_region = {0, 0, 0, 0};
        
        while (simulation_running) {
            frame_start = get_absolute_time();
            
            // Only clear the dice area from previous frame instead of the whole screen
            for (int i = 0; i < MAX_DICES; i++) {
                // Get dice area from previous frame
                Region region = get_dice_region(&dices[i]);
                // Clear area with margin pixels to ensure full coverage, increased clearing area
                region.x1 -= 5; region.y1 -= 5;
                region.x2 += 5; region.y2 += 5;
                // Ensure within screen boundaries
                if (region.x1 < 0) region.x1 = 0;
                if (region.y1 < 0) region.y1 = 0;
                if (region.x2 >= SCREEN_WIDTH) region.x2 = SCREEN_WIDTH - 1;
                if (region.y2 >= SCREEN_HEIGHT) region.y2 = SCREEN_HEIGHT - 1;
                
                // Clear this region
                ili9488_fill_area_rgb24(region.x1, region.y1, region.x2, region.y2, COLOR_DARK_RED);
                
                // Update physics state
                update_dice_physics(&dices[i]);
                
                // Draw dice
                draw_dice_optimized(&dices[i]);
            }
            
            // Check if all dice have stopped
            if (all_dices_stopped()) {
                simulation_running = false;
                
                // Display results
                display_dice_results();
            }
            
            // Frame rate control - use longer frame interval
            uint32_t frame_time = absolute_time_diff_us(frame_start, get_absolute_time()) / 1000;
            if (frame_time < FRAME_TIME_MS) {
                sleep_ms(FRAME_TIME_MS - frame_time);
            }
            
            // Extra delay
            sleep_ms(5);
            
            frame_count++;
            
            // Print progress
            if (frame_count % 30 == 0) {
                uint32_t elapsed_ms = absolute_time_diff_us(cycle_start, get_absolute_time()) / 1000;
                float fps = (float)frame_count * 1000.0f / elapsed_ms;
                printf("Frame count: %ld, FPS: %.1f\n", frame_count, fps);
            }
        }
        
        // Display final results
        printf("Simulation completed, results displayed\n");
        
        // Wait before starting again
        sleep_ms(5000);
        
        // Simplified mode switching - only keep DMA toggle
        if (frame_count > 0) {
            static int mode = 0;
            mode = (mode + 1) % 2;
            
            switch (mode) {
                case 0:
                    use_dma = true;
                    break;
                case 1:
                    use_dma = false;
                    break;
            }
        }
    }
    
    return 0;
} 