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
 *
 * 支持的目标画面类型:
 *   TGT_METER_BAR  — 进度条型仪表盘 (meter_bar)
 *   TGT_METER_QUAD — 四象限型仪表盘 (meter_quad)
 *   TGT_CUSTOM     — 自定义渲染画面
 */
#include "user_func.h"
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
    TGT_METER_BAR,
    TGT_METER_QUAD,
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

    /* 待启动的仪表盘页面 */
    const meter_bar_page_t  *pending_meter_bar_page;
    const meter_quad_page_t *pending_meter_quad_page;

    /* 自定义界面 */
    bool              in_custom_screen;
    uint8_t           custom_screen_id;
    UI_screen_render_cb  custom_render_cb;

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
    meter_bar_state_t  meter_bar;
    meter_quad_state_t meter_quad;

    /* 弹窗注册表 */
    popup_setup_fn   popup_setups[UI_POPUP_MAX];
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
    case TGT_METER_BAR:
        meter_bar_open(&a->meter_bar, a->pending_meter_bar_page);
        break;
    case TGT_METER_QUAD:
        meter_quad_open(&a->meter_quad, a->pending_meter_quad_page);
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

void UI_popup_value_open(const char *title, int16_t *val,
                       int16_t min, int16_t max, int16_t step) {
    popup_value_open(&g.value_popup, title, val, min, max, step);
}

void UI_popup_toggle_open(const char *title, bool *val,
                        const char *on, const char *off) {
    popup_toggle_open(&g.toggle_popup, title, val, on, off);
}

void UI_popup_toast_show(const char *text) {
    popup_toast_show(&g.toast, text);
}

void UI_screen_set_render(UI_screen_render_cb render) {
    g.custom_render_cb = render;
}

void UI_popup_confirm_open(const char *text, UI_popup_confirm_cb on_result) {
    popup_confirm_open(&g.confirm, text, (confirm_callback_t)on_result);
}

void UI_menu_goto_root(void) {
    g.menu.current   = g.menu_root;
    g.menu.selected  = 0;
    anim_set_position(&g.menu.scroll_anim, 0, 0);
    g.menu.scroll_target = 0;
    g.menu.bar_target_y  = -1;
    g.menu.bar_target_w  = -1;
}

void UI_popup_register(popup_setup_fn setup) {
    if (g.popup_setup_count < UI_POPUP_MAX)
        g.popup_setups[g.popup_setup_count++] = setup;
}

/* ---- 画面切换入口 ---- */

void UI_screen_enter(uint8_t id) {
    if (!g.custom_render_cb) return;
    g.custom_screen_id = id;
    g.scr_target       = TGT_CUSTOM;
    g.scr_state        = SCR_MENU_EXITING;
    menu_trans_out(&g.menu, on_menu_exit_done, &g);
}

void UI_meter_bar_open(const meter_bar_page_t *page) {
    g.pending_meter_bar_page = page;
    g.scr_target             = TGT_METER_BAR;
    g.scr_state              = SCR_MENU_EXITING;
    menu_trans_out(&g.menu, on_menu_exit_done, &g);
}

void UI_meter_quad_open(const meter_quad_page_t *page) {
    g.pending_meter_quad_page = page;
    g.scr_target              = TGT_METER_QUAD;
    g.scr_state               = SCR_MENU_EXITING;
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

void UI_init(u8g2_t *u8g2, const menu_page_t *root) {
    (void)u8g2;
    memset(&g, 0, sizeof(g));
    g.menu_root = root;
    g.scr_state = SCR_MENU;

    splash_init(&g.splash);
    menu_init(&g.menu, root);
    meter_bar_init(&g.meter_bar);
    meter_quad_init(&g.meter_quad);
    popup_mgr_init();

    UI_popup_register(setup_value);
    UI_popup_register(setup_toggle);
    UI_popup_register(setup_toast);
    UI_popup_register(setup_confirm);

    for (uint8_t i = 0; i < g.popup_setup_count; i++)
        g.popup_setups[i]();
}

void UI_update(int8_t key) {
    /* 所有画面每帧更新 (推进各自的动画/状态机) */
    splash_update(&g.splash);
    menu_update(&g.menu);
    meter_bar_update(&g.meter_bar);
    meter_quad_update(&g.meter_quad);

    if (!splash_done(&g.splash)) return;

    /* ---- 画面调度 ---- */
    switch (g.scr_state) {

    case SCR_MENU_EXITING:
        /* 等待菜单退出动画完成 → on_menu_exit_done 回调推进 */
        return;

    case SCR_TARGET_ENTERING:
        /* 目标画面的入场动画 */
        if (g.scr_target == TGT_METER_BAR && g.meter_bar.trans == METER_BAR_ACTIVE)
            g.scr_state = SCR_TARGET_ACTIVE;
        else if (g.scr_target == TGT_METER_QUAD && g.meter_quad.trans == METER_QUAD_ACTIVE)
            g.scr_state = SCR_TARGET_ACTIVE;
        /* TGT_CUSTOM 在 on_menu_exit_done 中直接跳到 ACTIVE */
        /* 入场期间屏蔽按键 */
        return;

    case SCR_TARGET_ACTIVE:
        /* 目标画面活跃: 仅 Back 退出 */
        popup_mgr_update(key);                    /* Toast 等叠加弹窗仍响应 */
        if (key == 4) {
            if (g.scr_target == TGT_METER_BAR) {
                meter_bar_close(&g.meter_bar);
                g.scr_state = SCR_TARGET_EXITING;
            } else if (g.scr_target == TGT_METER_QUAD) {
                meter_quad_close(&g.meter_quad);
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
        if ((g.scr_target == TGT_METER_BAR  && g.meter_bar.trans == METER_BAR_IDLE) ||
            (g.scr_target == TGT_METER_QUAD && g.meter_quad.trans == METER_QUAD_IDLE)) {
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

void UI_render(u8g2_t *u8g2) {
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
        if (g.scr_target == TGT_METER_BAR) {
            meter_bar_render(&g.meter_bar, u8g2);
        } else if (g.scr_target == TGT_METER_QUAD) {
            meter_quad_render(&g.meter_quad, u8g2);
        } else if (g.scr_target == TGT_CUSTOM && g.custom_render_cb) {
            g.custom_render_cb(u8g2, g.custom_screen_id);
        }
        break;
    }

    /* 弹窗始终在最上层 */
    popup_mgr_render(u8g2);
    u8g2_SendBuffer(u8g2);
}
