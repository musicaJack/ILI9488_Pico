# ILI9488_Pico

[中文文档](./README_zh.md)

ILI9488 TFT-LCD display driver library and examples for Raspberry Pi Pico.

## Project Introduction

This project provides an ILI9488 TFT-LCD driver library for Raspberry Pi Pico, using SPI interface to communicate with the display. ILI9488 is a common 3.5-inch 320x480 resolution TFT display controller.

Main features:
- Support for 18-bit color mode (RGB666)
- Support for multiple rotation directions
- Provides basic graphics drawing API (points, lines, rectangles, circles, etc.)
- Support for text display
- Support for Chinese character display
- Provides hardware abstraction layer for cross-platform portability
- Includes example code

## Architecture Design

This project adopts a layered architecture design:

1. **Hardware Abstraction Layer (HAL)** - Handles direct interaction with hardware, such as SPI communication, GPIO operations, etc.
2. **Driver Layer** - Implements the driver functionality for the ILI9488 controller
3. **Graphics Layer** - Provides basic graphics drawing functions
4. **Font Layer** - Handles text and character display

This layered design makes the code more modular, easier to maintain and port to other platforms.

## Hardware Connection

Please connect the Raspberry Pi Pico to the ILI9488 display as follows:

| Raspberry Pi Pico | ILI9488 LCD |
|-------------------|-------------|
| GPIO 19 (SPI0 TX) | MOSI (SDA)  |
| GPIO 18 (SPI0 SCK)| SCK (SCL)   |
| GPIO 17           | CS          |
| GPIO 20           | DC (RS)     |
| GPIO 15           | RESET       |
| GPIO 10           | BL (Backlight)|
| 3.3V              | VCC         |
| GND               | GND         |

Hardware connection photo:

![Hardware connection photo](imgs/hardware1-1.jpg)

## File Structure

- `/include` - Header files
  - `ili9488.h` - ILI9488 driver header file
  - `ili9488_hal.h` - Hardware abstraction layer header file
  - `ili9488_gfx.h` - Graphics function library header file
- `/src` - Source files
  - `ili9488_hal.c` - Hardware abstraction layer implementation (platform-specific)
  - `ili9488.c` - ILI9488 driver implementation (platform-independent)
  - `ili9488_gfx.c` - Basic graphics drawing functions
  - `ili9488_font.c` - Font and text drawing functions
- `/examples` - Example programs
  - `ili9488_demo.c` - Demo program
- `CMakeLists.txt` - CMake build file
- `pico_sdk_import.cmake` - Pico SDK import script

## Compilation and Running

### Environment Setup

1. Ensure Raspberry Pi Pico SDK and related toolchains are installed
2. Set the PICO_SDK_PATH environment variable to point to the SDK location

### Build Steps

On Windows:

```bash
mkdir build
cd build
cmake -G "NMake Makefiles" ..
nmake
```

Or use the provided build script:

```bash
./build_pico.bat
```

On Linux/Mac:

```bash
mkdir build
cd build
cmake ..
make
```

### Flashing to Pico

1. Hold the BOOTSEL button on the Pico while connecting the USB
2. Drag and drop the generated .uf2 file to the RPI-RP2 drive that appears

Or use the provided deployment script:

```bash
./deploy_to_pico.bat
```

## Using the API

### Initializing the Display

```c
// Configure LCD
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
    .rotation = 0,  // 0 degree rotation
};

// Initialize LCD
if (!ili9488_init(&config)) {
    printf("Error: LCD initialization failed\n");
    return -1;
}

// Turn on backlight
ili9488_set_backlight(true);
```

### Basic Drawing

```c
// Fill screen
ili9488_fill_screen(ILI9488_RED);

// Draw pixel
ili9488_draw_pixel(10, 10, ILI9488_WHITE);

// Draw line
ili9488_draw_line(0, 0, 100, 100, ILI9488_GREEN);

// Draw rectangle
ili9488_draw_rect(50, 50, 100, 80, ILI9488_BLUE);
ili9488_fill_rect(60, 60, 80, 60, ILI9488_YELLOW);

// Draw circle
ili9488_draw_circle(160, 120, 40, ILI9488_CYAN);
ili9488_fill_circle(160, 120, 30, ILI9488_MAGENTA);
```

### Text Display

```c
// Draw string
ili9488_draw_string(10, 10, "Hello, World!", ILI9488_WHITE, ILI9488_BLACK, 2);

// Draw Chinese characters (font data required)
ili9488_draw_chinese(50, 50, 0, ILI9488_RED, chines_word);
```

## Porting to Other Platforms

To port this driver to other platforms, you only need to reimplement the hardware-related functions in the `ili9488_hal.c` file, while keeping the other files unchanged.

## License

This project is open-sourced under the MIT license.

## References

- [ILI9488 Datasheet](http://www.lcdwiki.com/res/DevBoard/ILI9488%20DataSheet%2020150415.pdf)
- [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf) 