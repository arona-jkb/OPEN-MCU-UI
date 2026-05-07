# GUI_demo — STM32F103 单色 OLED GUI 框架

基于 STM32CubeMX 生成的 CMake 工程，在 SSD1306 128x64 OLED 上实现多级菜单、弹窗对话框与平滑动画。

---

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F103C8T6 (Cortex-M3, 64 KB Flash, 20 KB RAM) |
| 显示屏 | SSD1306 128x64 单色 OLED |
| 接口 | I2C（地址 0x78，硬件 I2C1, PB6/PB7） |
| 按键 | 4 键，上拉输入 (PC13 / PA0 / PA1 / PA2) |

## 开发环境

| 组件 | 版本 / 说明 |
|------|-------------|
| STM32CubeMX | **6.15.0** (DB.6.0.150) — 生成 HAL 驱动、CMSIS、启动代码 |
| STM32F1xx HAL 驱动 | **1.1.10** |
| CMSIS-Core (M) | **5.0.2** |
| 工具链 | **GNU Tools for STM32 13.3.1** (`arm-none-eabi-gcc`) |
| 构建系统 | **CMake 3.22+** + **Ninja** 生成器 |
| CMake 预设 | `cmake/gcc-arm-none-eabi.cmake` 工具链文件 |
| C 标准 | C11（带扩展） |
| IDE 支持 | VS Code + STM32Cube IDE 扩展 (`cube-cmake` / `starm-clangd`) |

## 构建

```bash
# 配置（Debug）
cmake --preset Debug

# 配置（Release）
cmake --preset Release

# 构建
cmake --build build/Debug
# 或
cmake --build build/Release
```

> 构建需要 `arm-none-eabi-gcc` 在 PATH 中可见。VS Code 环境下由 `cube-cmake` 自动配置工具链路径。

## 项目结构

```
GUI_demo/
├── Core/
│   ├── Inc/
│   │   ├── menu.h          # 菜单系统 API
│   │   ├── popup.h         # 弹窗系统 API (数值 / 布尔 / Toast)
│   │   ├── ux_move.h       # 动画引擎 API
│   │   ├── Key.h           # 按键输入
│   │   ├── stm32_u8g2.h    # u8g2 硬件适配层
│   │   └── main.h          # CubeMX 生成的主头文件
│   ├── Src/
│   │   ├── main.c          # 入口 + 主循环
│   │   ├── menu.c          # 菜单实现
│   │   ├── popup.c         # 弹窗实现
│   │   ├── ux_move.c       # 动画引擎实现
│   │   ├── Key.c           # 按键去抖动
│   │   ├── stm32_u8g2.c    # I2C 回调 + 微秒延迟
│   │   └── *.c             # CubeMX 生成的外设初始化
│   └── u8g2/               # u8g2 完整库 (~43 个 .c 文件)
├── Drivers/                 # STM32 HAL + CMSIS
├── cmake/
│   ├── gcc-arm-none-eabi.cmake   # GCC 工具链文件
│   ├── starm-clang.cmake         # ST ARM Clang 工具链文件（备选）
│   └── stm32cubemx/CMakeLists.txt # CubeMX 生成的 HAL/启动文件
├── CMakeLists.txt           # 根 CMake（添加用户源文件在此）
├── CMakePresets.json        # Debug / Release 预设
├── GUI_demo.ioc             # STM32CubeMX 项目文件
└── STM32F103XX_FLASH.ld     # 链接脚本
```

## 模块说明

### 菜单系统 (`menu.h`)

多级层次菜单，支持页面切换动画。核心结构：

- **`menu_item_t`** — 单个菜单项：显示名称 + action 回调 + 子菜单指针
- **`menu_page_t`** — 菜单页：标题 + 菜单项数组 + 父页面指针
- **`menu_state_t`** — 运行时状态，包含选择索引、滚动动画、选择条动画、页面过渡动画

页面切换时旧页的标题栏向上飞出、文字向下沉出，新页从反方向汇入。过渡期间按键被屏蔽。

### 弹窗系统 (`popup.h`)

三种弹窗类型，共享统一的滑入/滑出动画：

| 类型 | 用途 | 交互 |
|------|------|------|
| `popup_num_t` | 数值调节 | 上下键增减，确认/返回关闭 |
| `popup_bool_t` | 布尔开关 | 上下键翻转，确认/返回关闭 |
| `popup_toast_t` | 自动通知 | 1 秒后自动消失，无交互 |

### 动画引擎 (`ux_move.h`)

基于整数坐标插值的轻量动画框架：

- `anim_ctrl_t` 控制块 — 状态机: IDLE → PLAYING → FINISHED
- 支持 `quad_ease_out`（二次缓出）和 `linear_ease`（线性）两种缓动函数
- 最多 10 个并发动画（`MAX_ANIM_NUM`）
- 支持步骤序列、循环、暂停/恢复、反向播放

### 按键输入 (`Key.h`)

四按键，上拉，通过"松手瞬间检测"去抖动。`Key_Tick()` 由 SysTick ISR 每 20 ms 调用一次。

### u8g2 适配 (`stm32_u8g2.h`)

- SSD1306 128x64，全帧缓冲模式 (`_f`)
- 硬件 I2C: `HAL_I2C_Master_Transmit`
- 微秒延迟: TIM1 自由运行计数器
- 字体: 菜单用 `helvB10_tr`，标题栏用 `6x10_tr`

## 添加用户源文件

在根 `CMakeLists.txt`（非 `cmake/stm32cubemx/CMakeLists.txt`）的 USER 区域添加：

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/your_file.c
)
```

## CubeMX 代码标记

生成的文件中包含 `/* USER CODE BEGIN ... */` / `/* USER CODE END ... */` 标记。在这些标记之间编写的代码在 CubeMX 重新生成时会被**保留**；标记之外的代码会被覆盖。

## API 参考

完整 API 文档参见 `GUI_Framework_API_Reference_zh.pdf`（同目录下）。

## 代码来源

### 原作者编写（接管项目前已存在）

| 文件 | 说明 |
|------|------|
| `Core/Src/ux_move.c` / `Core/Inc/ux_move.h` | 动画引擎 |
| `Core/Src/Key.c` / `Core/Inc/Key.h` | 按键驱动（去抖动 + 状态读取） |
| `Core/Src/stm32_u8g2.c` / `Core/Inc/stm32_u8g2.h` | u8g2 硬件适配层（I2C + 延迟） |
| `Core/u8g2/` | u8g2 图形库（43 个 .c 文件，约 1945 套字体） |
| `Core/Src/stm32f1xx_it.c` | 中断服务（含 SysTick → Key_Tick 接线） |
| `Core/Src/main.c` | 原始主循环（滚动菜单原型） |
| `ST M32F103XX_FLASH.ld` | 链接脚本 |
| `startup_stm32f103xb.s` | 启动文件（向量表 + BSS 清零） |
| `cmake/` | CMake 工具链 + CubeMX 子 CMakeLists |
| `.vscode/` | VS Code 配置（cube-cmake / starm-clangd） |
| `CMakeLists.txt` / `CMakePresets.json` | 构建系统 |
| `GUI_demo.ioc` | CubeMX 项目文件 |
| `Drivers/` | STM32 HAL 1.1.10 + CMSIS 5.0.2 |

### Claude Code 生成（接管后新增）

| 文件 | 说明 |
|------|------|
| `Core/Inc/menu.h` + `Core/Src/menu.c` | **菜单系统** — 多级菜单、页面切换动画、XOR 选择条 |
| `Core/Inc/popup.h` + `Core/Src/popup.c` | **弹窗系统** — 数值调节 / 布尔开关 / Toast 通知三种弹窗 |
| `CLAUDE.md` | Claude Code 项目指引 |
| `README.md` | 本文件 |
| `GUI_Framework_API_Reference_zh.pdf` | 中文 API 参考手册 |
| `generate_docs.py` | PDF 文档生成脚本 |

### 被 Claude Code 大幅修改的已有文件

| 文件 | 变更内容 |
|------|----------|
| `Core/Src/main.c` | 主循环完全重写：集成菜单、弹窗、自定义界面、动画管理；新增 5 个菜单页和 5 个 action 回调 |
| `CMakeLists.txt` | 新增 `menu.c`、`popup.c` 到构建 |

## 许可

基于 STM32CubeMX 生成，使用 STM32 HAL 驱动。u8g2 库采用 BSD 许可。
