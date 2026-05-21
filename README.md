# OPEN MCU UI — STM32F103 单色 OLED GUI 框架

基于 STM32CubeMX 生成的 CMake 工程，在 SSD1306 128x64 OLED 上实现多级菜单、弹窗对话框与平滑动画。

> **代码归属**：动画引擎 (`ux_move`)、按键驱动 (`Key`)、u8g2 硬件适配层 (`stm32_u8g2`) 为**原作者**编写。菜单系统 (`menu`)、弹窗系统 (`popup`)、启动动画 (`splash`)、应用对接层 (`app_ui`) 及动画参数集中管理 (`ui_timing`) 由 **Claude Code** 生成。`main.c` 经 Claude Code 重构为极简主循环。

---

## 1. 移植依赖

将本 UI 框架移植到其他 STM32 工程时，需要满足以下环境：

| 依赖项 | 版本 / 要求 | 说明 |
|------|-------------|------|
| MCU 系列 | STM32F1xx (Cortex-M3) | 其他系列需确认 HAL 兼容性 |
| STM32CubeMX | **6.15.0** | 生成 HAL 驱动、CMSIS、启动代码 |
| STM32F1xx HAL | **1.1.10** | `Drivers/STM32F1xx_HAL_Driver/` |
| CMSIS-Core (M) | **5.0.2** | `Drivers/CMSIS/` |
| 工具链 | GNU Tools for STM32 **13.3.1**+ | `arm-none-eabi-gcc` |
| CMake | **3.22**+ | 配合 Ninja 生成器 |
| u8g2 图形库 | 完整源码 (~43 个 `.c`) | 需放在 `Core/u8g2/`, 自动 glob |
| 编译器标准 | **C11** (带 GNU 扩展) | `-std=gnu11` |
| 链接脚本 | `STM32F103XX_FLASH.ld` | 栈 ≥ 0x400, 堆 ≥ 0x200 |
| SysTick | 1 kHz | `HAL_IncTick()` 必须在 `SysTick_Handler` 中调用 |

### 最小硬件要求

| 硬件 | 规格 |
|------|------|
| MCU | STM32F103C8T6 (64 KB Flash, 20 KB RAM) |
| 显示 | SSD1306 128x64 单色 OLED, I2C 地址 `0x78` |
| 按键 | 4 键, 上拉输入 (PC13 / PA0 / PA1 / PA2) |
| 定时器 | TIM1 自由运行计数器, 用于 `delay_us()` |

### 外设初始化顺序（由 CubeMX 生成，不可打乱）

```
HAL_Init() → SystemClock_Config() → HAL_SYSTICK_Config(HCLK/1000)
→ MX_GPIO_Init() → MX_DMA_Init() → MX_TIM1_Init() → MX_SPI1_Init()
→ HAL_TIM_Base_Start(&htim1) → u8g2Init(&u8g2)
```

> `HAL_SYSTICK_Config` 必须在时钟配置**之后**调用，否则 SysTick 频率错误会导致动画异常。
> `HAL_TIM_Base_Start` 必须在 `u8g2Init` 之前，因为 u8g2 初始化过程中会调用 `delay_us()`。

---

## 2. 构建

```bash
cmake --preset Debug       # 配置
cmake --build build/Debug  # 构建
```

> 构建需要 `arm-none-eabi-gcc` 在 PATH 中可见。

---

## 3. 项目结构

```
GUI_demo/
├── Core/
│   ├── UI/
│   │   ├── Inc/
│   │   │   ├── menu.h / popup.h / ux_move.h / splash.h / Key.h
│   │   │   ├── stm32_u8g2.h
│   │   │   ├── app_ui.h        ← 开发者入口
│   │   │   └── ui_timing.h     ← 集中管理动画时长
│   │   └── Src/
│   │       ├── menu.c / popup.c / ux_move.c / splash.c / Key.c
│   │       ├── stm32_u8g2.c
│   │       └── app_ui.c        ← 内部编排逻辑
│   ├── Inc/                    ← CubeMX 生成的头文件
│   ├── Src/
│   │   └── main.c              ← 开发者工作区（极简主循环）
│   └── u8g2/                   ← u8g2 完整库
├── Drivers/                    ← HAL + CMSIS
├── cmake/                      ← 工具链 + CubeMX 子 CMake
├── CMakeLists.txt              ← 根构建（添加用户源文件在此）
└── GUI_demo.ioc                ← CubeMX 项目文件
```

---

## 4. 快速开始

### 4.1 主循环（`main.c`）

最终主循环只需三行核心代码：

```c
#include "app_ui.h"
#include "Key.h"

int main(void) {
    // ... HAL 及外设初始化 (见 §1) ...

    app_ui_init(&u8g2, &root_page);
    app_ui_set_custom_render(my_custom_render);

    while (1) {
        anim_manager_update();
        int8_t key = Key();
        app_ui_update(key);
        app_ui_render(&u8g2);
    }
}
```

### 4.2 定义菜单

```c
// 前向声明 action 回调
static void bright_action(void);
static void toggle_action(void);

// 子菜单页
static const menu_item_t settings_items[] = {
    {"Brightness", bright_action, NULL},
    {"Power Save", toggle_action, NULL},
};
static menu_page_t settings_page = {
    .title = "Settings", .items = settings_items,
    .count = 2, .parent = NULL,
};

// 根菜单
static menu_item_t root_items[] = {
    {"Settings", NULL, &settings_page},
};
static menu_page_t root_page = {
    .title = "Main Menu", .items = root_items,
    .count = 1, .parent = NULL,
};

// 连接父页面指针 (在 main 中)
settings_page.parent = &root_page;
```

### 4.3 编写 action 回调

```c
static int16_t brightness = 50;
static bool    power = true;

static void bright_action(void) {
    app_ui_value_open("Brightness", &brightness, 0, 100, 5);
}

static void toggle_action(void) {
    app_ui_toggle_open("Power Save", &power, "ON", "OFF");
}
```

### 4.4 自定义界面

```c
// 绘制回调
static void my_custom_render(u8g2_t *u8g2, uint8_t id) {
    u8g2_SetFontMode(u8g2, 1);
    if (id == 1) {
        u8g2_DrawStr(u8g2, 10, 30, "Screen 1");
    } else {
        u8g2_DrawStr(u8g2, 10, 30, "Screen 2");
    }
}

// action 回调
static void screen1_action(void) { app_ui_custom_screen_enter(1); }
```

---

## 5. 开发者 API 参考

### 弹窗

| 函数 | 用途 |
|------|------|
| `app_ui_value_open(title, *val, min, max, step)` | 打开数值调节弹窗 |
| `app_ui_toggle_open(title, *val, "ON", "OFF")` | 打开开关弹窗 |
| `app_ui_toast_show("text")` | 弹出 1 秒自动消失通知 |

### 导航

| 函数 | 用途 |
|------|------|
| `app_ui_custom_screen_enter(id)` | 进入自定义界面 (id 由开发者定义) |
| `app_ui_set_custom_render(fn)` | 注册自定义界面绘制回调 |
| `app_ui_goto_root()` | 从任意层级直接返回根菜单 |

### 动画调参

所有动画时长集中在 `Core/UI/Inc/ui_timing.h`，修改后重新构建即可生效：

```c
#define BAR_ANIM_MS      200    // 选择条移动
#define SCROLL_ANIM_MS   350    // 菜单文字滚动
#define TRANS_MS         800    // 页面切换
#define POPUP_OPEN_MS    300    // 弹窗滑入
#define TOAST_DURATION   1000   // Toast 停留
#define SPLASH_ENTER_MS  500    // 启动动画飞入
```

---

## 6. CubeMX 代码标记

生成文件中 `/* USER CODE BEGIN ... */` / `/* USER CODE END ... */` 之间的代码在 CubeMX 重新生成时**保留**；标记之外会被覆盖。**所有功能代码请写在标记内。**

---

## 7. 代码归属

| 模块 | 作者 | 文件 |
|------|------|------|
| 动画引擎 | **原作者** | `Core/UI/Inc/ux_move.h`, `Core/UI/Src/ux_move.c` |
| 按键驱动 | **原作者** | `Core/UI/Inc/Key.h`, `Core/UI/Src/Key.c` |
| u8g2 适配层 | **原作者** | `Core/UI/Inc/stm32_u8g2.h`, `Core/UI/Src/stm32_u8g2.c` |
| u8g2 图形库 | u8g2 社区 | `Core/u8g2/` |
| CubeMX 生成文件 | STM32CubeMX | `Core/Inc/`, `Core/Src/` (除 main.c), `Drivers/`, `cmake/stm32cubemx/` |
| 菜单系统 | **Claude Code** | `Core/UI/Inc/menu.h`, `Core/UI/Src/menu.c` |
| 弹窗系统 | **Claude Code** | `Core/UI/Inc/popup.h`, `Core/UI/Src/popup.c` |
| 启动动画 | **Claude Code** | `Core/UI/Inc/splash.h`, `Core/UI/Src/splash.c` |
| 应用对接层 | **Claude Code** | `Core/UI/Inc/app_ui.h`, `Core/UI/Src/app_ui.c` |
| 动画参数 | **Claude Code** | `Core/UI/Inc/ui_timing.h` |
| 主循环 | **原作者** + Claude Code 重构 | `Core/Src/main.c` |

---

## 8. 许可

STM32 HAL 驱动版权归 STMicroelectronics 所有。u8g2 库采用 BSD 许可。
