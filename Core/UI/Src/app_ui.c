/**
 * UI 框架主控
 *
 * 画面调度状态机 (screen state machine):
 *
 *   SCR_MENU  ←────────────┐
 *     │ (action=外部画面)    │ (外部 → 菜单)
 *     ↓                     │
 *   SCR_MENU_EXITING        │
 *     │ (menu 退场完成)      │
 *     ↓                     │
 *   SCR_TARGET_ENTERING ──┐ │
 *     │ (目标入场完成)      │ │
 *     ↓                   │ │
 *   SCR_TARGET_ACTIVE      │ │
 *     │ (Back 键)          │ │
 *     ↓                   │ │
 *   SCR_TARGET_EXITING ──┘ │
 *     │ (目标退场完成)      │
 *     ↓                    │
 *   SCR_MENU_ENTERING ─────┘
 *
 * 渲染优先级: Splash → External Screen → Menu → Popups
 */
#include "app_ui.h"
#include "popup.h"
#include "popup_confirm.h"
#include "splash.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ================================================================
 *  画面状态机
 * ================================================================ */

typedef enum {
    SCR_MENU,               /* 菜单活跃 */
    SCR_MENU_EXITING,       /* 菜单播放退出动画 (→ 外部画面)      */
    SCR_TARGET_ENTERING,    /* 目标画面播放入场动画                 */
    SCR_TARGET_ACTIVE,      /* 目标画面活跃                         */
    SCR_TARGET_EXITING,     /* 目标画面播放退场动画 (→ 菜单)       */
    SCR_MENU_ENTERING,      /* 菜单播放入场动画 (← 外部画面)       */
} screen_state_e;

typedef enum {
    TGT_NONE,
    TGT_METER,
    TGT_CUSTOM,
} target_type_e;

/* ================================================================
 *  全局状态
 * ================================================================ */

typedef struct {
    splash_t          splash;
    menu_state_t      menu;
    const menu_page_t *menu_root;

    /* 画面调度 */
    screen_state_e     scr_state;
    target_type_e      scr_target;
    const meter_page_t *pending_meter_page;

    /* 自定义界面 */
    bool              in_custom_screen;
    uint8_t           custom_screen_id;
    app_ui_render_fn  custom_render_cb;

    /* 弹窗 */
    popup_value_t    value_popup;
    popup_base_t     value_base;
    popup_toggle_t   toggle_popup;
    popup_base_t     toggle_base;
    popup_toast_t    toast;
    popup_base_t     toast_base;
    popup_confirm_t  confirm;
    popup_base_t     confirm_base;

    /* 仪表盘 */
    meter_state_t    meter;

    /* 弹窗注册表 */
    popup_setup_fn   popup_setups[APP_UI_MAX_POPUPS];
    uint8_t          popup_setup_count;
} app_state_t;

static app_state_t g;

/* ================================================================
 *  内部回调 — 画面调度
 * ================================================================ */

/* 菜单退出完成: 启动目标画面入场 */
static void on_menu_exit_done(void *ctx) {
    app_state_t *a = (app_state_t *)ctx;
    a->scr_state = SCR_TARGET_ENTERING;
    switch (a->scr_target) {
    case TGT_METER:
        meter_open(&a->meter, a->pending_meter_page);
        break;
    case TGT_CUSTOM:
        /* 自定义界面无入场动画, 直接活跃 */
        a->scr_state      = SCR_TARGET_ACTIVE;
        a->in_custom_screen = true;
        break;
    default: break;
    }
}

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

/* ---- 画面切换入口 ---- */

void app_ui_custom_screen_enter(uint8_t id) {
    if (!g.custom_render_cb) return;
    g.custom_screen_id = id;
    g.scr_target       = TGT_CUSTOM;
    g.scr_state        = SCR_MENU_EXITING;
    menu_trans_out(&g.menu, on_menu_exit_done, &g);
}

void app_ui_meter_open(const meter_page_t *page) {
    g.pending_meter_page = page;
    g.scr_target         = TGT_METER;
    g.scr_state          = SCR_MENU_EXITING;
    menu_trans_out(&g.menu, on_menu_exit_done, &g);
}

/* ---- 内置弹窗注册回调 ---- */
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
    memset(&g, 0, sizeof(g));
    g.menu_root = root;
    g.scr_state = SCR_MENU;

    splash_init(&g.splash);
    menu_init(&g.menu, root);
    meter_init(&g.meter);
    popup_mgr_init();

    app_ui_register_popup(setup_value);
    app_ui_register_popup(setup_toggle);
    app_ui_register_popup(setup_toast);
    app_ui_register_popup(setup_confirm);

    for (uint8_t i = 0; i < g.popup_setup_count; i++)
        g.popup_setups[i]();
}

void app_ui_update(int8_t key) {
    /* 所有画面每帧更新 (推进各自的动画/状态机) */
    splash_update(&g.splash);
    menu_update(&g.menu);
    meter_update(&g.meter);

    if (!splash_done(&g.splash)) return;

    /* ---- 画面调度 ---- */
    switch (g.scr_state) {

    case SCR_MENU_EXITING:
        /* 等待菜单退出动画完成 → on_menu_exit_done 回调推进 */
        return;

    case SCR_TARGET_ENTERING:
        /* 目标画面的入场动画 */
        if (g.scr_target == TGT_METER && g.meter.trans == METER_ACTIVE)
            g.scr_state = SCR_TARGET_ACTIVE;
        /* TGT_CUSTOM 在 on_menu_exit_done 中直接跳到 ACTIVE */
        /* 入场期间屏蔽按键 */
        return;

    case SCR_TARGET_ACTIVE:
        /* 目标画面活跃: 仅 Back 退出 */
        popup_mgr_update(key);                    /* Toast 等叠加弹窗仍响应 */
        if (key == 4) {
            if (g.scr_target == TGT_METER) {
                meter_close(&g.meter);
                g.scr_state = SCR_TARGET_EXITING;
            } else if (g.scr_target == TGT_CUSTOM) {
                g.in_custom_screen = false;
                menu_trans_in(&g.menu);
                g.scr_state = SCR_MENU_ENTERING;
            }
        }
        return;

    case SCR_TARGET_EXITING:
        /* 目标画面退场中 */
        if (g.scr_target == TGT_METER && g.meter.trans == METER_IDLE) {
            /* 仪表盘退场完成 → 菜单入场 */
            menu_trans_in(&g.menu);
            g.scr_state = SCR_MENU_ENTERING;
        }
        return;

    case SCR_MENU_ENTERING:
        /* 菜单入场中 */
        if (g.menu.trans == TRANS_NONE)
            g.scr_state = SCR_MENU;
        return;

    case SCR_MENU:
        /* 菜单正常运作 */
        popup_mgr_update(key);

        if (!popup_mgr_any_active()) {
            if (key == 1)      menu_key_up(&g.menu);
            else if (key == 2) menu_key_down(&g.menu);
            else if (key == 3) menu_key_enter(&g.menu);
            else if (key == 4) menu_key_back(&g.menu);
        }
        return;
    }
}

void app_ui_render(u8g2_t *u8g2) {
    /* Splash */
    if (!splash_done(&g.splash)) {
        splash_render_frame(&g.splash, u8g2, menu_render_wrap, &g.menu);
        return;
    }

    u8g2_ClearBuffer(u8g2);

    /* 根据画面状态选择渲染对象 */
    switch (g.scr_state) {

    case SCR_MENU:
    case SCR_MENU_EXITING:
    case SCR_MENU_ENTERING:
        /* 菜单渲染 (内部根据 trans 状态绘制过渡或正常画面) */
        menu_render(u8g2, &g.menu);
        break;

    case SCR_TARGET_ENTERING:
    case SCR_TARGET_ACTIVE:
    case SCR_TARGET_EXITING:
        if (g.scr_target == TGT_METER) {
            meter_render(&g.meter, u8g2);
        } else if (g.scr_target == TGT_CUSTOM && g.custom_render_cb) {
            g.custom_render_cb(u8g2, g.custom_screen_id);
        }
        break;
    }

    /* 弹窗始终在最上层 */
    popup_mgr_render(u8g2);
    u8g2_SendBuffer(u8g2);
}
