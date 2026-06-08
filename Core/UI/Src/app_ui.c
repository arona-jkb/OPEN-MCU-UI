#include "app_ui.h"
#include "popup.h"
#include "popup_confirm.h"
#include "splash.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ================================================================
 *  全局状态 — 统一收敛到 app_state_t
 * ================================================================ */

typedef struct {
    splash_t          splash;
    menu_state_t      menu;
    const menu_page_t *menu_root;
    bool              in_custom_screen;
    uint8_t           custom_screen_id;
    app_ui_render_fn  custom_render_cb;

    /* 内置弹窗实例 (后续新增类型同样在此追加) */
    popup_value_t  value_popup;
    popup_base_t   value_base;
    popup_toggle_t toggle_popup;
    popup_base_t   toggle_base;
    popup_toast_t  toast;
    popup_base_t   toast_base;
    popup_confirm_t confirm;
    popup_base_t    confirm_base;

    /* 弹窗注册表 */
    popup_setup_fn  popup_setups[APP_UI_MAX_POPUPS];
    uint8_t         popup_setup_count;
} app_state_t;

static app_state_t g;   /* 唯一的全局实例 */

/* ================================================================
 *  公开 API
 * ================================================================ */

void app_ui_value_open(const char *title, int16_t *val,
                       int16_t min, int16_t max, int16_t step) {
    popup_value_open(&g.value_popup, title, val, min, max, step);
}

void app_ui_toggle_open(const char *title, bool *val,
                        const char *on, const char *off) {
    popup_toggle_open(&g.toggle_popup, title, val, on, off);
}

void app_ui_toast_show(const char *text) {
    popup_toast_show(&g.toast, text);
}

void app_ui_custom_screen_enter(uint8_t id) {
    if (g.custom_render_cb) {
        g.custom_screen_id = id;
        g.in_custom_screen = true;
    }
}

void app_ui_set_custom_render(app_ui_render_fn render) {
    g.custom_render_cb = render;
}

void app_ui_confirm_open(const char *text, app_ui_confirm_fn on_result) {
    popup_confirm_open(&g.confirm, text, (confirm_callback_t)on_result);
}

void app_ui_goto_root(void) {
    g.menu.current   = g.menu_root;
    g.menu.selected  = 0;
    anim_set_position(&g.menu.scroll_anim, 0, 0);
    g.menu.scroll_target = 0;
    g.menu.bar_target_y  = -1;
    g.menu.bar_target_w  = -1;
}

void app_ui_register_popup(popup_setup_fn setup) {
    if (g.popup_setup_count < APP_UI_MAX_POPUPS)
        g.popup_setups[g.popup_setup_count++] = setup;
}

/* ---- 内置弹窗注册回调 (未来新增类型在此追加) ---- */
static void setup_value(void)   { popup_value_init(&g.value_popup, &g.value_base); }
static void setup_toggle(void)  { popup_toggle_init(&g.toggle_popup, &g.toggle_base); }
static void setup_toast(void)   { popup_toast_init(&g.toast, &g.toast_base); }
static void setup_confirm(void) { popup_confirm_init(&g.confirm, &g.confirm_base); }

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
    memset(&g, 0, sizeof(g));             /* 所有字段归零 */
    g.menu_root = root;

    splash_init(&g.splash);
    menu_init(&g.menu, root);
    popup_mgr_init();

    /* 注册内置弹窗 */
    app_ui_register_popup(setup_value);
    app_ui_register_popup(setup_toggle);
    app_ui_register_popup(setup_toast);
    app_ui_register_popup(setup_confirm);

    /* 执行注册表中所有弹窗的初始化 */
    for (uint8_t i = 0; i < g.popup_setup_count; i++)
        g.popup_setups[i]();
}

/*
 *  按键分发优先级:
 *    启动动画       → 不响应按键
 *    自定义界面     → 仅 Back 键退出
 *    弹窗激活       → 消费全部按键
 *    菜单           → 正常导航
 */
void app_ui_update(int8_t key) {
    splash_update(&g.splash);
    menu_update(&g.menu);

    if (!splash_done(&g.splash)) return;

    if (g.in_custom_screen) {
        if (key == 4) g.in_custom_screen = false;
        return;
    }

    popup_mgr_update(key);

    if (!popup_mgr_any_active()) {
        if (key == 1)      menu_key_up(&g.menu);
        else if (key == 2) menu_key_down(&g.menu);
        else if (key == 3) menu_key_enter(&g.menu);
        else if (key == 4) menu_key_back(&g.menu);
    }
}

/*
 *  渲染优先级:
 *    启动动画 → 自定义界面 → 菜单(+弹窗)
 */
void app_ui_render(u8g2_t *u8g2) {
    if (!splash_done(&g.splash)) {
        splash_render_frame(&g.splash, u8g2, menu_render_wrap, &g.menu);
        return;
    }

    if (g.in_custom_screen && g.custom_render_cb) {
        u8g2_ClearBuffer(u8g2);
        g.custom_render_cb(u8g2, g.custom_screen_id);
        u8g2_SendBuffer(u8g2);
        return;
    }

    u8g2_ClearBuffer(u8g2);
    menu_render(u8g2, &g.menu);
    popup_mgr_render(u8g2);
    u8g2_SendBuffer(u8g2);
}
