# CollisionX Game - ILI9488版本

## 项目概述

这是原本基于ST7789显示屏的CollisionX游戏的ILI9488适配版本。游戏已完全重写以充分利用ILI9488的更大分辨率(480x320)和现代C++ API。

## 硬件要求

### 必需硬件
- **Raspberry Pi Pico** 开发板
- **ILI9488 3.5寸 TFT显示屏** (480x320分辨率)
- **I2C摇杆模块** (兼容原joystick_Pico项目)

### 引脚连接

#### ILI9488显示屏连接
| Pico引脚 | ILI9488引脚 | 功能 |
|----------|-------------|------|
| GPIO18   | SCK         | SPI时钟 |
| GPIO19   | MOSI        | SPI数据 |
| GPIO17   | CS          | 片选信号 |
| GPIO20   | DC          | 数据/命令选择 |
| GPIO15   | RST         | 复位信号 |
| GPIO10   | BL          | 背光控制 |
| 3V3      | VCC         | 电源正极 |
| GND      | GND         | 电源负极 |

#### 摇杆模块连接  
| Pico引脚 | 摇杆引脚 | 功能 |
|----------|----------|------|
| GPIO6    | SDA      | I2C数据线 |
| GPIO7    | SCL      | I2C时钟线 |
| 3V3      | VCC      | 电源正极 |
| GND      | GND      | 电源负极 |

## 游戏特性

### 改进和优化
- **更大的游戏区域**: 从240x320扩展到480x320，提供更宽敞的游戏空间
- **增强的图形效果**: 更大的方块(30x30)和更粗的边界线(8像素)  
- **优化的移动**: 更快的移动步长(8像素)适应更大屏幕
- **改进的速度平衡**: 小球速度范围调整为-4到4，适应更大的游戏区域
- **现代化API**: 使用模板化的图形引擎和类型安全的接口

### 游戏规则
1. **目标**: 使用摇杆控制放置蓝色方块，保护绿球在20秒内不撞到上下边界
2. **控制方式**:
   - 摇杆移动: 控制光标移动
   - 短按中键: 放置蓝色防护方块
   - 在已有方块上短按: 升级为铁方块(灰色)
   - 长按中键3秒: 释放小球开始游戏
3. **方块类型**:
   - **蓝色方块**: 普通防护方块，需要2次碰撞消失
   - **铁方块**: 强化方块，需要8次碰撞消失  
   - **小绿球**: 主要保护目标
   - **小黄球**: 特殊球，对铁方块只需6次碰撞，对普通方块1次碰撞

### LED指示
- 🟢 **绿色**: 初始化成功
- 🔵 **蓝色**: 摇杆操作中  
- 🔴 **红色**: 按键操作瞬间(50ms)
- ⚫ **关闭**: 无操作/空闲状态

## 技术实现

### 核心技术栈
- **显示驱动**: Modern C++ ILI9488 Driver (RGB565/RGB888支持)
- **图形引擎**: 模板化PicoILI9488GFX系统
- **摇杆驱动**: I2C通信的Joystick类
- **颜色管理**: 完整的RGB565/RGB888/RGB666颜色转换

### 性能优化
- **模板特化**: 编译时优化的图形渲染
- **批量绘制**: 高效的区域填充和像素操作
- **智能刷新**: 只重绘变化的区域
- **防抖算法**: 稳定的输入处理(需3次连续读数确认)

### 代码结构
```cpp
// 核心类型定义
struct BlockPosition { int16_t x, y; };
struct WanderingDot { BlockPosition pos; int16_t speed_x, speed_y; bool active, is_yellow; };

// 模板化绘制函数
template<typename Driver>
void drawBlock(PicoILI9488GFX<Driver>& gfx, const BlockPosition& pos, bool is_stamp, bool is_iron);

// 高效的碰撞检测
int checkCollisionDirection(const BlockPosition& pos, const StampPositions& stamps, uint8_t& hit_index);
```

## 编译和烧录

### 前提条件
```bash
# 确保已安装Pico SDK
export PICO_SDK_PATH=/path/to/pico-sdk
```

### 编译步骤
```bash
mkdir build
cd build
cmake ..
make CollisionX
```

### 烧录
1. 按住Pico的BOOTSEL按钮，连接USB
2. 将生成的`CollisionX.uf2`文件复制到出现的RPI-RP2驱动器
3. 设备将自动重启并运行游戏

## 调试输出

通过USB串口可以看到详细的调试信息:
```
Joystick and ILI9488 LCD Integration Demo
Initialization successful!
mid(1)
mid(2)
Convert to iron block
Reached maximum stamps limit (50)
```

## 故障排除

### 常见问题
1. **显示屏不亮**: 检查SPI连接和背光引脚
2. **摇杆无响应**: 检查I2C连接和地址(0x63)
3. **游戏卡顿**: 确保电源供电充足
4. **颜色显示异常**: 检查SPI速度设置(建议40MHz)

### 调试技巧
- 使用串口监视器查看初始化状态
- 检查LED指示确认摇杆工作状态
- 验证引脚连接是否正确

## 扩展可能

这个版本为进一步开发提供了良好的基础:
- 添加更多游戏关卡
- 实现高分记录系统  
- 增加音效支持
- 开发多人游戏模式
- 添加更多特殊方块类型

## 许可证

基于原项目MIT许可证发布。 