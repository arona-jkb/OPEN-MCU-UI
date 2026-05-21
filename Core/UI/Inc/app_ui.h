#ifndef __APP_UI_H__
#define __APP_UI_H__

#include "u8g2.h"
#include "menu.h"
#include <stdint.h>
#include <stdbool.h>

/* 自定义界面绘制回调 — 开发者用 id 分发多套界面 */
typedef void (*app_ui_render_fn)(u8g2_t *u8g2, uint8_t id);

/* ---------- 生命周期 ---------- */

void app_ui_init(u8g2_t *u8g2, const menu_page_t *root);
void app_ui_update(int8_t key);           /* 按键分发 (splash → 自定义界面 → 弹窗 → 菜单) */
void app_ui_render(u8g2_t *u8g2);         /* ClearBuffer → 渲染 → SendBuffer */

/* ---------- 开发者钩子 ---------- */

void app_ui_set_custom_render(app_ui_render_fn render);
void app_ui_custom_screen_enter(uint8_t id);

/* ---------- 弹窗快捷函数 (在 action 回调中使用) ---------- */

void app_ui_value_open(const char *title, int16_t *val,
                       int16_t min, int16_t max, int16_t step);
void app_ui_toggle_open(const char *title, bool *val,
                        const char *on, const char *off);
void app_ui_toast_show(const char *text);
void app_ui_goto_root(void);

#endif
