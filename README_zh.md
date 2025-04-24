# ILI9488_Pico

ILI9488 TFT-LCD 显示驱动库和示例，适用于 Raspberry Pi Pico。

## 项目介绍

本项目提供了用于 Raspberry Pi Pico 的 ILI9488 TFT-LCD 驱动库，使用 SPI 接口与显示屏通信。ILI9488 是一款常见的 3.5 英寸 320x480 分辨率 TFT 显示控制器。

主要特性：
- 支持 18 位颜色模式 (RGB666)
- 支持多种旋转方向
- 提供基本图形绘制API（点、线、矩形、圆形等）
- 支持文本显示
- 支持中文字符显示
- 提供硬件抽象层，便于跨平台移植
- 包含示例代码

## 架构设计

本项目采用分层架构设计：

1. **硬件抽象层 (HAL)** - 处理与硬件直接交互的部分，如SPI通信、GPIO操作等
2. **驱动层** - 实现ILI9488控制器的驱动功能
3. **图形层** - 提供基本的图形绘制功能
4. **字体层** - 处理文本和字符显示

这种分层设计使得代码更加模块化，便于维护和移植到其他平台。

## 硬件连接

请按照以下方式连接 Raspberry Pi Pico 与 ILI9488 显示屏：

| Raspberry Pi Pico | ILI9488 LCD |
|-------------------|-------------|
| GPIO 19 (SPI0 TX) | MOSI (SDA)  |
| GPIO 18 (SPI0 SCK)| SCK (SCL)   |
| GPIO 17           | CS          |
| GPIO 20           | DC (RS)     |
| GPIO 15           | RESET       |
| GPIO 10           | BL (背光)    |
| 3.3V              | VCC         |
| GND               | GND         |

## 文件结构

- `/include` - 头文件
  - `ili9488.h` - ILI9488 驱动头文件
  - `ili9488_hal.h` - 硬件抽象层头文件
  - `ili9488_gfx.h` - 图形函数库头文件
- `/src` - 源文件
  - `ili9488_hal.c` - 硬件抽象层实现（平台相关）
  - `ili9488.c` - ILI9488 驱动实现（与平台无关）
  - `ili9488_gfx.c` - 基本图形绘制函数
  - `ili9488_font.c` - 字体和文本绘制函数
- `/examples` - 示例程序
  - `ili9488_demo.c` - 演示程序
- `CMakeLists.txt` - CMake 构建文件
- `pico_sdk_import.cmake` - Pico SDK 导入脚本

## 编译和运行

### 环境设置

1. 确保已安装 Raspberry Pi Pico SDK 和相关工具链
2. 设置 PICO_SDK_PATH 环境变量指向 SDK 位置

### 构建步骤

在 Windows 中使用:

```bash
mkdir build
cd build
cmake -G "NMake Makefiles" ..
nmake
```

或者使用提供的构建脚本:

```bash
./build_pico.bat
```

在 Linux/Mac 中使用:

```bash
mkdir build
cd build
cmake ..
make
```

### 烧录到 Pico

1. 按住 Pico 上的 BOOTSEL 按钮的同时连接 USB
2. 将生成的 .uf2 文件拖放到出现的 RPI-RP2 驱动器中

或者使用提供的部署脚本:

```bash
./deploy_to_pico.bat
```

## 使用 API

### 初始化显示屏

```c
// 配置 LCD
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
    .rotation = 0,  // 0度旋转
};

// 初始化 LCD
if (!ili9488_init(&config)) {
    printf("错误: LCD初始化失败\n");
    return -1;
}

// 打开背光
ili9488_set_backlight(true);
```

### 基本绘图

```c
// 填充屏幕
ili9488_fill_screen(ILI9488_RED);

// 绘制像素
ili9488_draw_pixel(10, 10, ILI9488_WHITE);

// 绘制线
ili9488_draw_line(0, 0, 100, 100, ILI9488_GREEN);

// 绘制矩形
ili9488_draw_rect(50, 50, 100, 80, ILI9488_BLUE);
ili9488_fill_rect(60, 60, 80, 60, ILI9488_YELLOW);

// 绘制圆
ili9488_draw_circle(160, 120, 40, ILI9488_CYAN);
ili9488_fill_circle(160, 120, 30, ILI9488_MAGENTA);
```

### 文本显示

```c
// 绘制字符串
ili9488_draw_string(10, 10, "Hello, World!", ILI9488_WHITE, ILI9488_BLACK, 2);

// 绘制中文字符 (需要提供字库数据)
ili9488_draw_chinese(50, 50, 0, ILI9488_RED, chines_word);
```

## 移植到其他平台

如果要将本驱动移植到其他平台，只需重新实现`ili9488_hal.c`文件中的硬件相关函数，而保持其他文件不变即可。

## 许可证

本项目基于 MIT 许可证开源。

## 参考

- [ILI9488 数据表](http://www.lcdwiki.com/res/DevBoard/ILI9488%20DataSheet%2020150415.pdf)
- [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf) 