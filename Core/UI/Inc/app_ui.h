#ifndef __APP_UI_H__
#define __APP_UI_H__

#include "u8g2.h"
#include "menu.h"
#include "meter.h"
#include <stdint.h>
#include <stdbool.h>

typedef void (*app_ui_confirm_fn)(bool ok);
typedef void (*app_ui_render_fn)(u8g2_t *u8g2, uint8_t id);
typedef void (*popup_setup_fn)(void);

#define APP_UI_MAX_POPUPS  6

/* ---------- 生命周期 ---------- */

void app_ui_init(u8g2_t *u8g2, const menu_page_t *root);
void app_ui_update(int8_t key);
void app_ui_render(u8g2_t *u8g2);

/* ---------- 弹窗注册 ---------- */

void app_ui_register_popup(popup_setup_fn setup);

/* ---------- 画面切换 ---------- */

void app_ui_custom_screen_enter(uint8_t id);
void app_ui_set_custom_render(app_ui_render_fn render);
void app_ui_meter_open(const meter_page_t *page);

/* ---------- 弹窗快捷函数 ---------- */

void app_ui_value_open(const char *title, int16_t *val,
                       int16_t min, int16_t max, int16_t step);
void app_ui_toggle_open(const char *title, bool *val,
                        const char *on, const char *off);
void app_ui_toast_show(const char *text);
void app_ui_confirm_open(const char *text, app_ui_confirm_fn on_result);
void app_ui_goto_root(void);

#endif
