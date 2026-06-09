# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

STM32F103xB (Cortex-M3) embedded GUI demo project, driving an SSD1306 128x64 OLED via I2C, using the u8g2 graphics library. Generated from STM32CubeMX, built with CMake + Ninja.

## Build Commands

```bash
# Configure (Debug)
cmake --preset Debug

# Configure (Release)
cmake --preset Release

# Build
cmake --build build/Debug
# or
cmake --build build/Release
```

The build uses the `arm-none-eabi-` GCC toolchain (`cmake/gcc-arm-none-eabi.cmake`). A ST ARM Clang toolchain file exists as alternative (`cmake/starm-clang.cmake`).

The IDE (`cube-cmake` / `starm-clangd`) is configured via `.vscode/settings.json` and requires the STM32Cube IDE bundle environment.

## UI Architecture

The UI framework follows a **screen dispatcher** pattern. The menu system is the skeleton; everything else is a pluggable full-screen component.

### Screen State Machine (`user_func.c`)

```
SCR_MENU ←────────────────────────┐
  │ (action triggers external screen) │
  ↓                                  │
SCR_MENU_EXITING  (menu exit anim)   │
  │ (on_menu_exit_done callback)     │
  ↓                                  │
SCR_TARGET_ENTERING (target's enter) │
  │ (enter animation done)           │
  ↓                                  │
SCR_TARGET_ACTIVE  (target active)   │
  │ (Back key pressed)               │
  ↓                                  │
SCR_TARGET_EXITING (target's exit)   │
  │ (exit animation done)            │
  ↓                                  │
SCR_MENU_ENTERING (menu re-enter) ───┘
  │ (enter animation done)
  ↓
SCR_MENU
```

**Key files:**

| File | Role |
|------|------|
| `Core/UI/Src/app_ui.c` | Screen dispatcher — the single point of control for all screen transitions |
| `Core/UI/Src/menu.c` | Text & icon menu system (two styles: MENU_TEXT / MENU_ICON) |
| `Core/UI/Src/popup.c` | Popup manager + value/toggle/toast popup implementations |
| `Core/UI/Src/popup_confirm.c` | Confirm dialog (OK/Cancel) |
| `Core/UI/Src/meter.c` | Dashboard component (labeled progress bars) |
| `Core/UI/Src/splash.c` | Boot/splash animation |
| `Core/UI/Src/anim_engine.c` | Animation engine |
| `Core/UI/Src/Key.c` | Key input driver |

### Rendering Priority

```
Splash → External Screen (meter/custom) → Menu → Popups
```

Popups always render on top, regardless of which screen is active. Toast popups can overlay meters, confirm dialogs can overlay menus, etc.

### How to Add a New Screen Type

1. **Create header + source** (`meter.h` / `meter.c`) following this pattern:

```c
// State enum with at least: IDLE, ENTER, ACTIVE, EXIT
typedef enum { X_IDLE, X_ENTER, X_ACTIVE, X_EXIT } x_trans_e;

typedef struct {
    x_trans_e trans;
    anim_ctrl_t /* entry/exit animations */;
    // ... page data pointer
} x_state_t;

// Required API:
void x_init(x_state_t *s);
void x_open(x_state_t *s, const x_page_t *page);
void x_close(x_state_t *s);       // start exit animation
void x_update(x_state_t *s);      // per-frame, advance state machine
void x_render(const x_state_t *s, u8g2_t *u8g2);
bool x_active(const x_state_t *s);
```

2. **Add `target_type_e` entry** in `user_func.c`

3. **Add the internal callback** to `on_menu_exit_done`

4. **Register in `app_state_t`** and call `x_init()` in `app_ui_init()`

5. **Add cases** to `UI_update` (SCR_TARGET_ENTERING / ACTIVE / EXITING) and `UI_render`

6. **Provide a public API** in `user_func.h` (e.g. `app_ui_x_open()`)

### Transition Design Philosophy

- **Menu → Submenu**: Handled internally by `menu.c` (TRANS_OLD_OUT → TRANS_NEW_IN). Each item has its own animation slot.
- **Menu → External Screen**: `menu_trans_out()` starts menu exit. When done, `trans_cb` callback fires, which starts the target's entry animation. The callback mechanism in `menu_update()` checks `state->trans_cb` after TRANS_OLD_OUT completes.
- **External Screen → Menu**: `target.close()` starts target exit. When done, `menu_trans_in()` starts menu re-enter. When that completes, back to SCR_MENU.
- **Custom screen**: Uses the same menu exit/enter transition, but the custom screen itself has no animation (pops in/out instantly).

### Key API Summary (developer-facing, from `user_func.h`)

| Function | Purpose |
|----------|---------|
| `app_ui_init(u8g2, root)` | Initialize framework with root menu page |
| `app_ui_update(key)` | Per-frame update + key dispatch |
| `app_ui_render(u8g2)` | Full frame: ClearBuffer → render → SendBuffer |
| `app_ui_value_open(...)` | Open value adjuster popup |
| `app_ui_toggle_open(...)` | Open toggle switch popup |
| `app_ui_toast_show(text)` | Show auto-dismiss notification |
| `app_ui_confirm_open(text, cb)` | Show OK/Cancel confirm dialog |
| `app_ui_meter_open(page)` | Switch to meter dashboard |
| `app_ui_custom_screen_enter(id)` | Switch to custom render screen |
| `app_ui_set_custom_render(fn)` | Register custom screen render callback |
| `app_ui_register_popup(fn)` | Register additional popup type |

### Menu Definition Macros

```c
MENU_PAGE_TEXT("title", &parent_page,  /* title left-aligned, vertical list */
    { "Item", {0}, action_cb, NULL },
    { "Sub",  {0}, NULL, &sub_page },
);

MENU_PAGE_ICON("title", &parent_page,  /* title centered, horizontal icons */
    { "Home", {icon_bits, 24, 24}, NULL, &sub_page },
);
```

### Source File Rules

- **`CMakeLists.txt` (root)**: Add user `.c` sources, include paths, libraries. Not overwritten by CubeMX.
- **`cmake/stm32cubemx/CMakeLists.txt`**: Auto-generated by CubeMX. HAL drivers, CMSIS, startup, peripheral init. Regenerated when `.ioc` is modified.

### u8g2 Integration

Full u8g2 library in `Core/u8g2/`, auto-globbed by root CMakeLists.txt. Configured for SSD1306 I2C (address `0x78`). Hardware adaptation: `Core/Src/stm32_u8g2.c`.

### Animation System (`Core/Src/anim_engine.c`)

- `anim_ctrl_t` state machine: `IDLE → PLAYING → FINISHED` (also `PAUSED`, `BACKING`)
- `anim_manager_update()` — must be called each main-loop iteration
- `MAX_ANIM_NUM` = 25 (tuned for MCU RAM, icon menu transitions need many slots)
- Easing functions: `quad_ease_out`, `linear_ease`
- Step sequences, loop flag supported. Auto-register/unregister from global manager.

### Key Input (`Core/Src/Key.c`)

4-button input (PC13, PA0, PA1, PA2) with pull-up. `Key_Tick()` every ~20ms latches a key number; `Key()` reads and clears.

### Pin Definitions (from main.h)

| Pin  | Label     | Function    |
|------|-----------|-------------|
| PC13 | user_key  | Button 4    |
| PA0  | key1      | Button 1    |
| PA1  | key2      | Button 2    |
| PA2  | key3      | Button 3    |

### STM32CubeMX Code Markers

`/* USER CODE BEGIN ... */` / `/* USER CODE END ... */` markers preserve user code across CubeMX regeneration.

### Component Inventory

| Component | Files | Style | Transition |
|-----------|-------|-------|------------|
| Text Menu | `menu.h/c` | Vertical list, animated selector bar | Page exit/enter (items fly up/down) |
| Icon Menu | `menu.h/c` | Horizontal icon row, center-aligned selection | Page exit/enter (items to/from center) |
| Value Popup | `popup.h/c` | Slider with numeric display | Slide in/out from top |
| Toggle Popup | `popup.h/c` | iOS-style switch | Slide in/out from top |
| Toast Popup | `popup.h/c` | Auto-dismiss notification | Slide in/out |
| Confirm Popup | `popup_confirm.h/c` | OK/Cancel with animated underline | Slide in/out |
| Meter | `meter.h/c` | Labeled progress bars | Title fly-in, rows slide-in stagger |
| Custom Screen | `user_func.h/c` | Developer-defined render callback | Menu exit/enter (no custom anim) |
| Splash | `splash.h/c` | Boot logo animation | Text fly-in, hold, fly-out |

### Naming Conventions

Public (developer-facing) APIs follow the pattern:

```
UI_component_subcomponent_action
```

| Prefix | Component | Example |
|--------|-----------|---------|
| `UI_` | Framework lifecycle | `UI_init`, `UI_update`, `UI_render` |
| `UI_menu_` | Menu navigation | `UI_menu_goto_root` |
| `UI_popup_` | Popup dialogs | `UI_popup_value_open`, `UI_popup_toast_show`, `UI_popup_confirm_open` |
| `UI_meter_` | Meter/dashboard | `UI_meter_open` |
| `UI_screen_` | Custom screen | `UI_screen_enter`, `UI_screen_set_render` |

Internal module functions use lowercase prefix with underscore: `menu_init`, `meter_render`, `popup_mgr_update`, `anim_start`.

Private file naming: lowercase with underscore. Public-entry file: `user_func.h` (was `app_ui.h`). Timing constants: `anitime_config.h`.
