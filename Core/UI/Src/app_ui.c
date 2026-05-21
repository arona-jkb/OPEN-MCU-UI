#include "app_ui.h"
#include "popup.h"
#include "splash.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* ================================================================
 *  内部全局状态 — 对 main.c 不可见
 * ================================================================ */

static splash_t          splash;
static menu_state_t      menu;
static const menu_page_t *menu_root;        /* 保存根页面指针, 供 goto_root 使用 */
static bool              in_custom_screen;  /* 是否处于自定义界面模式 */
static uint8_t           custom_screen_id;  /* 当前自定义界面编号 */
static app_ui_render_fn  custom_render_cb;  /* 开发者注册的绘制回调 */

/* 弹窗实例 — 通过 app_ui_* 快捷函数间接访问 */
static popup_value_t  value_popup;
static popup_base_t   value_base;
static popup_toggle_t toggle_popup;
static popup_base_t   toggle_base;
static popup_toast_t  toast;
static popup_base_t   toast_base;

/* ================================================================
 *  公开 API
 * ================================================================ */

void app_ui_value_open(const char *title, int16_t *val,
                       int16_t min, int16_t max, int16_t step) {
    popup_value_open(&value_popup, title, val, min, max, step);
}

void app_ui_toggle_open(const char *title, bool *val,
                        const char *on, const char *off) {
    popup_toggle_open(&toggle_popup, title, val, on, off);
}

void app_ui_toast_show(const char *text) {
    popup_toast_show(&toast, text);
}

void app_ui_custom_screen_enter(uint8_t id) {
    if (custom_render_cb) {
        custom_screen_id = id;
        in_custom_screen = true;
    }
}

void app_ui_set_custom_render(app_ui_render_fn render) {
    custom_render_cb = render;
}

void app_ui_goto_root(void) {
    menu.current   = menu_root;
    menu.selected  = 0;
    anim_set_position(&menu.scroll_anim, 0, 0);
    menu.scroll_target = 0;
    menu.bar_target_y  = -1;
    menu.bar_target_w  = -1;
}

/* ================================================================
 *  内部辅助
 * ================================================================ */

static void menu_render_wrap(void *ctx, u8g2_t *u8g2) {
    menu_render(u8g2, (menu_state_t *)ctx);
}

/* ================================================================
 *  生命周期
 * ================================================================ */

void app_ui_init(u8g2_t *u8g2, const menu_page_t *root) {
    (void)u8g2;
    menu_root = root;

    splash_init(&splash);
    menu_init(&menu, root);
    popup_mgr_init();
    popup_value_init(&value_popup, &value_base);
    popup_toggle_init(&toggle_popup, &toggle_base);
    popup_toast_init(&toast, &toast_base);
}

/*
 *  按键分发优先级:
 *    启动动画       → 不响应按键
 *    自定义界面     → 仅 Back 键退出
 *    弹窗激活       → 消费全部按键
 *    菜单           → 正常导航
 */
void app_ui_update(int8_t key) {
    splash_update(&splash);
    menu_update(&menu);

    if (!splash_done(&splash)) return;      /* 启动动画期间屏蔽一切 */

    if (in_custom_screen) {
        if (key == 4) in_custom_screen = false;
        return;
    }

    popup_mgr_update(key);                  /* 空闲弹窗内部直接返回 */

    if (!popup_mgr_any_active()) {
        if (key == 1)      menu_key_up(&menu);
        else if (key == 2) menu_key_down(&menu);
        else if (key == 3) menu_key_enter(&menu);
        else if (key == 4) menu_key_back(&menu);
    }
}

/*
 *  渲染优先级:
 *    启动动画 → 自定义界面 → 菜单(+弹窗)
 *  每帧完整执行 ClearBuffer → 绘制 → SendBuffer
 */
void app_ui_render(u8g2_t *u8g2) {
    if (!splash_done(&splash)) {
        splash_render_frame(&splash, u8g2, menu_render_wrap, &menu);
        return;
    }

    if (in_custom_screen && custom_render_cb) {
        u8g2_ClearBuffer(u8g2);
        custom_render_cb(u8g2, custom_screen_id);
        u8g2_SendBuffer(u8g2);
        return;
    }

    u8g2_ClearBuffer(u8g2);
    menu_render(u8g2, &menu);
    popup_mgr_render(u8g2);
    u8g2_SendBuffer(u8g2);
}
