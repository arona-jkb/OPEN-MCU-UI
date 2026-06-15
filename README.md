# OPEN MCU UI — STM32F103 单色 OLED GUI 框架

基于 STM32CubeMX + CMake 工程，在 SSD1306 128x64 OLED 上实现多级菜单、弹窗对话框、仪表盘与全场景平滑过渡动画。

> 本项目由笔者与 Claude Code 协作完成。动画引擎、按键驱动、u8g2 硬件适配层为笔者编写；菜单系统、弹窗系统、仪表盘组件、画面调度状态机、启动动画及整体架构由 Claude Code 参与设计与实现。

---

## 1. 特性一览

| 类别 | 功能 |
|------|------|
| **菜单** | 文字垂直列表 + 图标水平菜单（四角直角选中框），均带独立过渡动画 |
| **弹窗** | 数值调节、开关切换、Toast 通知、确认对话框，统一管理器调度 |
| **仪表盘** | 两种类型：进度条型 (标签+实时数值+动画进度条) 与四象限型 (纯数值大字+角标单位)，均为差异化入场动画 |
| **自定义界面** | 开发者自由绘制回调，Back 键返回菜单 |
| **过渡动画** | 菜单 ↔ 子菜单、菜单 ↔ 外部组件，全路径衔接，无硬切 |
| **启动动画** | "power by OPEN MCU UI" 飞入/静止/飞出 Splash |
| **动画引擎** | 轻量级坐标插值框架，支持缓动函数、序列、循环、回溯 |

---

## 2. 移植依赖

| 依赖项 | 版本 / 要求 | 说明 |
|------|-------------|------|
| MCU | STM32F1xx (Cortex-M3) | 其他系列需确认 HAL 兼容 |
| STM32CubeMX | 6.15.0 | 生成 HAL + CMSIS + 启动代码 |
| STM32F1xx HAL | 1.1.10 | `Drivers/STM32F1xx_HAL_Driver/` |
| 工具链 | GNU Arm Embedded 13.3.1+ | `arm-none-eabi-gcc` |
| CMake | 3.22+ | 配合 Ninja 生成器 |
| u8g2 | 完整源码 | 放于 `Core/u8g2/`，CMake 自动 glob |
| C 标准 | gn11 | `-std=gnu11` |
| SysTick | 1 kHz | `HAL_IncTick()` 须在 `SysTick_Handler` 调用 |

### 最小硬件

| 硬件 | 规格 |
|------|------|
| MCU | STM32F103C8T6 (64 KB Flash / 20 KB RAM) |
| 屏幕 | SSD1306 128×64 OLED, I2C `0x78` |
| 按键 | 4 键上拉输入 (PC13 / PA0 / PA1 / PA2) |
| 定时器 | TIM1 自由运行计数器（`delay_us()` 用） |

---

## 3. 构建

```bash
cmake --preset Debug
cmake --build build/Debug
```

![#](https://img.shields.io/badge/compiler-GCC%2013.3.1-blue)

---

## 4. 项目结构

```
GUI_demo/
├── Core/
│   ├── UI/
│   │   ├── Inc/
│   │   │   ├── user_func.h          ← 开发者唯一入口
│   │   │   ├── menu.h            ← 菜单数据结构 + 宏
│   │   │   ├── meter.h               ← 仪表盘 (bar 型 + quad 型) 数据 + API
│   │   │   ├── popup.h           ← 弹窗管理器 + value/toggle/toast
│   │   │   ├── popup_confirm.h   ← 确认对话框
│   │   │   ├── anim_engine.h         ← 动画引擎
│   │   │   ├── splash.h          ← 启动动画
│   │   │   ├── Key.h             ← 按键驱动
│   │   │   ├── stm32_u8g2.h      ← u8g2 硬件适配
│   │   │   └── anitime_config.h       ← 集中管理动画时长
│   │   └── Src/                  ← 对应 .c 实现
│   ├── Inc/                      ← CubeMX 生成 (main.h 等)
│   ├── Src/
│   │   └── main.c                ← 极简主循环
│   └── u8g2/                     ← u8g2 完整库
├── Drivers/                      ← HAL + CMSIS
├── cmake/                        ← 工具链 + CubeMX 子 CMake
├── CMakeLists.txt                ← 根构建文件
├── CMakePresets.json
└── GUI_demo.ioc                  ← CubeMX 项目文件
```

---

## 5. 快速开始

### 5.1 主循环

```c
#include "user_func.h"
#include "Key.h"

int main(void) {
    // HAL 及外设初始化 (略)

    UI_init(&u8g2, &root_page);
    UI_screen_set_render(my_custom_render);

    while (1) {
        anim_manager_update();
        int8_t key = Key();
        UI_update(key);
        UI_render(&u8g2);
    }
}
```

### 5.2 定义菜单

```c
// 前向声明 (宏内引用需先声明)
static void my_action(void);
static menu_page_t sub_page, root_page;

// 文字菜单
static menu_page_t sub_page =
    MENU_PAGE_TEXT("Sub Menu", &root_page,
        { "Item 1", {0}, my_action, NULL },
        { "Item 2", {0}, NULL,     NULL },
    );

// 图标菜单
static const uint8_t icon_a[] U8X8_PROGMEM = { ... };  // 24×24 XBM
static menu_page_t icon_page =
    MENU_PAGE_ICON("Tools", &root_page,
        { "Home", {icon_a, 24, 24}, NULL, &sub_page },
    );

// 根菜单 (parent = NULL)
static menu_page_t root_page =
    MENU_PAGE_TEXT("Main Menu", NULL,
        { "Sub Page",  {0}, NULL,          &sub_page },
        { "Icon Menu", {0}, NULL,          &icon_page },
        { "My Action", {0}, my_action,     NULL },
    );
```

### 5.3 编写回调

```c
static int16_t g_val = 50;
static bool    g_en  = true;

static void cb_value(void)  { UI_popup_value_open ("Value", &g_val, 0, 100, 10); }
static void cb_toggle(void) { UI_popup_toggle_open("Power", &g_en,  "ON", "OFF"); }
static void cb_toast(void)  { UI_popup_toast_show("Done."); }

static void cb_confirm_ok(bool ok) { if (ok) UI_popup_toast_show("OK!"); }
static void cb_confirm(void)       { UI_popup_confirm_open("Sure?", cb_confirm_ok); }
```

### 5.4 仪表盘

**进度条型 (meter_bar)** — 标签 + 实时数值 + 动画进度条：

```c
static int16_t g_temp = 23, g_humi = 67, g_volt = 330;

static meter_bar_page_t dash_bar =
    METER_BAR_PAGE("Bar Dashboard",
        { "Temperature", &g_temp, 0,   60,  "C",  5 },
        { "Humidity",    &g_humi, 0,  100,  "%",  5 },
        { "Voltage",     &g_volt, 200, 500, "mV", 5 },
    );

static void cb_dash_bar(void) { UI_meter_bar_open(&dash_bar); }
```

**四象限型 (meter_quad)** — 纯数值展示，大字居中突出，单位角标，2×2 网格分布：

```c
static int16_t g_temp = 30, g_humi = 67, g_press = 1013, g_rpm = 2750;

static meter_quad_page_t dash_quad =
    METER_QUAD_PAGE("Quad Monitor",
        { "Temperature", &g_temp,  "°C"  },
        { "Humidity",    &g_humi,  "%"   },
        { "Pressure",    &g_press, "hPa" },
        { "Motor RPM",   &g_rpm,   "rpm" },
    );

static void cb_dash_quad(void) { UI_meter_quad_open(&dash_quad); }
```

两种仪表盘都支持 0~4 项，实际渲染数量由宏自动计算。两者共用 Back 键退出，退场自动播放反向动画并返回菜单。

### 5.5 自定义界面

```c
static void my_render(u8g2_t *u8g2, uint8_t id) {
    u8g2_SetFontMode(u8g2, 1);
    u8g2_DrawStr(u8g2, 10, 30, id == 1 ? "Screen 1" : "Screen 2");
}

static void cb_screen1(void) { UI_screen_enter(1); }
```

---

## 6. 开发者 API 总览

### 画面切换

| 函数 | 说明 |
|------|------|
| `UI_screen_enter(id)` | 进入自定义界面（菜单退场 → 自定义即时出现） |
| `UI_meter_bar_open(page)` | 进入进度条型仪表盘（菜单退场 → 入场 stagger 动画） |
| `UI_meter_quad_open(page)` | 进入四象限型仪表盘（菜单退场 → 差异化方向入场动画） |

### 弹窗

| 函数 | 说明 |
|------|------|
| `UI_popup_value_open(title, *val, min, max, step)` | 数值调节弹窗 |
| `UI_popup_toggle_open(title, *val, on, off)` | 开关弹窗 |
| `UI_popup_toast_show(text)` | 1 秒自动消失通知 |
| `UI_popup_confirm_open(text, callback)` | 确认对话框 (OK/Cancel, 回调 bool) |

### 钩子与导航

| 函数 | 说明 |
|------|------|
| `UI_screen_set_render(fn)` | 注册自定义界面绘制回调 |
| `UI_menu_goto_root()` | 任意层级直接返回根菜单 |

---

## 7. 按键映射

| 键号 | 操作 | 菜单 | 图标菜单 | 弹窗 |
|------|------|------|---------|------|
| 1 | 上 | ↑ | ← (首项不循环) | value: +step / toggle: 翻转 |
| 2 | 下 | ↓ | → (末项不循环) | value: -step / toggle: 翻转 |
| 3 | 确认 | 执行 action / 进子菜单 | 同左 | 确认 |
| 4 | 返回 | 回父菜单 | 同左 | 取消/关闭 |

---

## 8. 画面调度架构

框架采用 **screen dispatcher** 状态机，菜单为骨架，外部画面为可插拔组件：

```
SCR_MENU ───action──▶ SCR_MENU_EXITING ──done──▶ SCR_TARGET_ENTERING
    ▲                                                    │ done
    │                                                    ▼
    │                                            SCR_TARGET_ACTIVE
    │                                                    │ Back
    │                                                    ▼
SCR_MENU ◀──done── SCR_MENU_ENTERING ◀──done─── SCR_TARGET_EXITING
```

弹窗独立于画面系统，始终渲染在最上层（Toast 可叠加于仪表盘上方）。

---

## 9. 动画调参

所有时长集中在一处修改：

| 文件 | 控制范围 |
|------|---------|
| `Core/UI/Inc/anitime_config.h` | 菜单、弹窗、Splash 全局时长 |
| `Core/UI/Src/menu.c` 宏 | 图标入场 stagger 延时 |
| `Core/UI/Src/meter.c` 宏 | 仪表盘 bar 型/quad 型入场退场时长与方向 |

---

## 10. CubeMX 代码标记

`/* USER CODE BEGIN ... */` / `/* USER CODE END ... */` 之间的代码在 CubeMX 重新生成时**保留**；标记之外会被覆盖。

---

## 11. 许可

STM32 HAL 驱动版权归 STMicroelectronics 所有。u8g2 库采用 BSD 许可。本项目 UI 框架部分可自由使用。
