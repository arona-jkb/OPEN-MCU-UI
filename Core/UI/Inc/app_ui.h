#ifndef __APP_UI_H__
#define __APP_UI_H__

#include "u8g2.h"
#include "menu.h"
#include "meter.h"

/* 确认对话框回调 — 与 popup_confirm.h 中一致 */
typedef void (*app_ui_confirm_fn)(bool ok);
#include <stdint.h>
#include <stdbool.h>

/* 自定义界面绘制回调 — 开发者用 id 分发多套界面 */
typedef void (*app_ui_render_fn)(u8g2_t *u8g2, uint8_t id);

/* 弹窗组件注册回调 — 每个弹窗类型提供一个, 在 app_ui_init 中调用 */
typedef void (*popup_setup_fn)(void);

#define APP_UI_MAX_POPUPS  6             /* 最大弹窗类型数 (value/toggle/toast...) */

/* ---------- 生命周期 ---------- */

void app_ui_init(u8g2_t *u8g2, const menu_page_t *root);
void app_ui_update(int8_t key);           /* 按键分发 (splash → 自定义界面 → 弹窗 → 菜单) */
void app_ui_render(u8g2_t *u8g2);         /* ClearBuffer → 渲染 → SendBuffer */

/* ---------- 弹窗注册 (在 app_ui_init 之前调用, 追加弹窗类型) ---------- */

void app_ui_register_popup(popup_setup_fn setup);

/* ---------- 开发者钩子 ---------- */

void app_ui_set_custom_render(app_ui_render_fn render);
void app_ui_custom_screen_enter(uint8_t id);

/* ---------- 弹窗快捷函数 (在 action 回调中使用) ---------- */

void app_ui_value_open(const char *title, int16_t *val,
                       int16_t min, int16_t max, int16_t step);
void app_ui_toggle_open(const char *title, bool *val,
                        const char *on, const char *off);
void app_ui_toast_show(const char *text);
void app_ui_confirm_open(const char *text, app_ui_confirm_fn on_result);
void app_ui_meter_open(const meter_page_t *page);
void app_ui_goto_root(void);

#endif
