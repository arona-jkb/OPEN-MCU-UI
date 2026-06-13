#ifndef __USER_FUNC_H__
#define __USER_FUNC_H__

#include "u8g2.h"
#include "menu.h"
#include "meter.h"
#include <stdint.h>
#include <stdbool.h>

typedef void (*UI_popup_confirm_cb)(bool ok);
typedef void (*UI_screen_render_cb)(u8g2_t *u8g2, uint8_t id);
typedef void (*popup_setup_fn)(void);

#define UI_POPUP_MAX  6

/* ---------- 生命周期 ---------- */

void UI_init(u8g2_t *u8g2, const menu_page_t *root);
void UI_update(int8_t key);
void UI_render(u8g2_t *u8g2);

/* ---------- 弹窗注册 ---------- */

void UI_popup_register(popup_setup_fn setup);

/* ---------- 画面切换 ---------- */

void UI_screen_enter(uint8_t id);
void UI_screen_set_render(UI_screen_render_cb render);

/* 仪表盘: bar 型 (进度条) */
void UI_meter_bar_open(const meter_bar_page_t *page);

/* 仪表盘: quad 型 (四象限数值) */
void UI_meter_quad_open(const meter_quad_page_t *page);

/* ---------- 弹窗快捷函数 ---------- */

void UI_popup_value_open(const char *title, int16_t *val,
                       int16_t min, int16_t max, int16_t step);
void UI_popup_toggle_open(const char *title, bool *val,
                        const char *on, const char *off);
void UI_popup_toast_show(const char *text);
void UI_popup_confirm_open(const char *text, UI_popup_confirm_cb on_result);
void UI_menu_goto_root(void);

#endif
