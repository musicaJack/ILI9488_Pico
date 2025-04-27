# NotoSans Font Library for RP2040 and ILI9488

This repository contains NotoSans font libraries generated with `bmc.py` tool for use with Raspberry Pi Pico (RP2040) and ILI9488 display (320x480).

## Overview

The font library provides bitmap fonts in different sizes for displaying text on ILI9488 displays. The fonts are optimized for embedded systems with limited resources.

## Generated Font Files

The following font header files are generated:

- `notosans_fonts.h` - Master include file containing all fonts
- `notosans_small_font.h` - Small size font
- `notosans_middle_font.h` - Medium size font
- `notosans_large_font.h` - Large size font
- `notosans_very_large_font.h` - Extra large size font

## Font Specifications

| Font Name | Font Size | Line Height | Characters |
|-----------|-----------|-------------|------------|
| Small     | 16px      | ~16px       | ASCII + Extended |
| Middle    | 24px      | ~24px       | ASCII + Extended |
| Large     | 32px      | ~32px       | ASCII + Extended |
| Very Large| 48px      | ~48px       | ASCII + Extended |

Each font supports ASCII characters and extended Unicode characters.

## API Reference

### Including the Font Library

```c
// Include all fonts
#include "notosans_fonts.h"

// Or include only the font you need
#include "notosans_small_font.h"
```

### Font Constants

Each font provides the following constants:

```c
// For small font (similar for other sizes)
#define NOTOSANS_SMALL_HEIGHT      // Font line height
#define NOTOSANS_SMALL_SIZE        // Font size
#define NOTOSANS_SMALL_BASE_LINE   // Font baseline position
#define NOTOSANS_SMALL_CHAR_COUNT  // Number of characters in the font
```

### Data Structures

Each font defines a character structure:

```c
typedef struct {
    uint16_t unicode;    // Unicode encoding
    uint8_t width;       // Character width
    uint8_t height;      // Character height
    int8_t xoffset;      // X offset
    int8_t yoffset;      // Y offset
    uint8_t xadvance;    // X advance
    const uint8_t *data; // Pointer to bitmap data
} notosans_small_char_t;  // Example for small font
```

### Functions

Each font provides a character lookup function:

```c
// Find character by Unicode value
const notosans_small_char_t* notosans_small_find_char(uint16_t unicode);
// Similar functions for other font sizes
```

## Usage Example

Example code files are provided for each font size:

- `ili9488_small_example.c`
- `ili9488_middle_example.c`
- `ili9488_large_example.c`
- `ili9488_very_large_example.c`

### Basic Drawing Example

```c
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "notosans_small_font.h"

// Initialize display
ili9488_init();

// Clear screen
ili9488_fill_rect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, COLOR_WHITE);

// Draw a simple text string
ili9488_draw_string(10, 10, "Hello World!", COLOR_BLACK, COLOR_WHITE);

// Draw a Unicode character (example: Euro symbol)
uint16_t euro_unicode = 0x20AC;
ili9488_draw_char(150, 50, euro_unicode, COLOR_RED, COLOR_WHITE);
```

## Hardware Configuration

The example code is configured for:

- **RP2040** (Raspberry Pi Pico)
- **ILI9488** display (320x480 resolution)
- **SPI Interface** configuration:
  - SCK: GPIO 2
  - MOSI: GPIO 3
  - DC: GPIO 4
  - CS: GPIO 5
  - RST: GPIO 6
  - BL: GPIO 7

You may need to adjust these pin assignments to match your hardware configuration.

## Font Generation

The font files were generated using the `bmc.py` script with BMfont-generated files:
- Input: NotoSans TrueType font converted to BMfont format
- Configuration settings: 8-bit, Run-length encoded, TGA format
- Multiple font sizes were generated for different use cases

## License

These font files are based on the NotoSans font, which is licensed under the Apache License, Version 2.0. 