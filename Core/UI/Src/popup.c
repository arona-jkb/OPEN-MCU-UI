/**
 * 弹窗模块实现
 *
 * 三种弹窗类型:
 *   popup_value_t  — 数值调节 (上下键增减, 带进度条)
 *   popup_toggle_t — 开关切换 (上下键翻转, 带滑块)
 *   popup_toast_t  — 自动通知 (1 秒后自动消失)
 *
 * 所有弹窗通过 popup_mgr_* 管理器统一调度,
 * 开发者只需调用 app_ui_*_open() / app_ui_toast_show().
 */
#include "popup.h"
#include "ui_timing.h"
#include <stdio.h>

/* ================================================================
 *  弹窗管理器 (全局单例)
 * ================================================================ */

static popup_base_t *g_popup_list[MAX_POPUP_NUM];
static uint8_t       g_popup_count;

void popup_mgr_init(void) {
    g_popup_count = 0;
}

bool popup_mgr_register(popup_base_t *p) {
    if (g_popup_count >= MAX_POPUP_NUM) return false;
    for (uint8_t i = 0; i < g_popup_count; i++)
        if (g_popup_list[i] == p) return true;   /* 已在列表中 */
    g_popup_list[g_popup_count++] = p;
    return true;
}

bool popup_mgr_any_active(void) {
    for (uint8_t i = 0; i < g_popup_count; i++)
        if (g_popup_list[i]->active(g_popup_list[i]->instance))
            return true;
    return false;
}

void popup_mgr_update(int8_t key) {
    for (uint8_t i = 0; i < g_popup_count; i++)
        g_popup_list[i]->update(g_popup_list[i]->instance, key);
}

void popup_mgr_render(u8g2_t *u8g2) {
    for (uint8_t i = 0; i < g_popup_count; i++)
        g_popup_list[i]->render(g_popup_list[i]->instance, u8g2);
}

/* ================================================================
 *  共享布局常数
 * ================================================================ */

#define POPUP_W     110                    /* 弹窗外宽            */
#define POPUP_H      40                    /* 弹窗外高            */
#define POPUP_R       4                    /* 圆角半径            */
#define POPUP_Y      ((64 - POPUP_H) / 2)  /* 竖直居中 = 12       */
#define POPUP_OFF    (-POPUP_H)            /* 屏幕上方隐藏位置    */

/* ================================================================
 *  数值调节弹窗
 *
 *  布局:
 *    标题(左上)  数值(右上 ~3/4 处)
 *    直角矩形进度条 (inset 2 px 填充)
 *  按键: Key1=+step  Key2=-step  Key3/4=关闭
 * ================================================================ */

/* 管理器回调包装 — 类型转换 + 调用静态函数 */
static void popup_value_update(popup_value_t *p, int8_t key);
static void popup_value_render(popup_value_t *p, u8g2_t *u8g2);

static bool val_active(void *p)       { return ((popup_value_t *)p)->state != POPUP_IDLE; }
static void val_update(void *p, int8_t key) { popup_value_update((popup_value_t *)p, key); }
static void val_render(void *p, u8g2_t *u8g2) { popup_value_render((popup_value_t *)p, u8g2); }

void popup_value_init(popup_value_t *p, popup_base_t *b) {
    p->state = POPUP_IDLE;
    anim_init(&p->slide);
    anim_init(&p->bar_anim);
    p->bar_target = -1;
    b->instance = p;
    b->active   = val_active;
    b->update   = val_update;
    b->render   = val_render;
    popup_mgr_register(b);               /* 自动注册到管理器 */
}

void popup_value_open(popup_value_t *p, const char *title, int16_t *value,
                      int16_t min, int16_t max, int16_t step) {
    p->cfg.title = title;
    p->cfg.value = value;
    p->cfg.min   = min;
    p->cfg.max   = max;
    p->cfg.step  = step;
    p->state     = POPUP_OPENING;
    p->bar_target = -1;
    p->saved      = *value;              /* 保存原始值, Key4 取消时还原 */
    anim_start(&p->slide, 0, POPUP_OFF, 0, POPUP_Y, POPUP_OPEN_MS, quad_ease_out);
}

static void popup_value_update(popup_value_t *p, int8_t key) {
    switch (p->state) {
    case POPUP_OPENING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_ACTIVE;
        break;
    case POPUP_ACTIVE:
        if (key == 1) {                  /* 上键: +step, 上限钳位 */
            *p->cfg.value += p->cfg.step;
            if (*p->cfg.value > p->cfg.max) *p->cfg.value = p->cfg.max;
        } else if (key == 2) {           /* 下键: -step, 下限钳位 */
            *p->cfg.value -= p->cfg.step;
            if (*p->cfg.value < p->cfg.min) *p->cfg.value = p->cfg.min;
        } else if (key == 3) {
            /* 确认 — 保留修改 */
            anim_start(&p->slide, 0, p->slide.cur_y, 0, POPUP_OFF,
                       POPUP_CLOSE_MS, quad_ease_out);
            p->state = POPUP_CLOSING;
        } else if (key == 4) {
            /* 取消 — 还原原始值 */
            *p->cfg.value = p->saved;
            anim_start(&p->slide, 0, p->slide.cur_y, 0, POPUP_OFF,
                       POPUP_CLOSE_MS, quad_ease_out);
            p->state = POPUP_CLOSING;
        }
        break;
    case POPUP_CLOSING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_IDLE;
        break;
    default: break;
    }
}

static void popup_value_render(popup_value_t *p, u8g2_t *u8g2) {
    if (p->state == POPUP_IDLE) return;
    int16_t py = p->slide.cur_y;
    int16_t px = (128 - POPUP_W) / 2;

    /* 黑色填充 — 遮住下层菜单 */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRBox(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);

    /* 标题 (左上) + 数值 (右上 3/4 处) */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    int16_t ttl_base = py + 13;
    u8g2_DrawStr(u8g2, px + 6, ttl_base, p->cfg.title);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", *p->cfg.value);
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    u8g2_uint_t w = u8g2_GetStrWidth(u8g2, buf);
    u8g2_DrawStr(u8g2, px + (POPUP_W * 3) / 4 - (int16_t)w / 2, ttl_base, buf);

    /* 进度条 (直角矩形, 填充与边框保持 2 px 间隔) */
    int16_t bar_x = px + 6,  bar_w = POPUP_W - 12;
    int16_t bar_h = 8,       bar_y = py + 22;
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawFrame(u8g2, bar_x, bar_y, bar_w, bar_h);

    /* 进度条填充 — 动画驱动 */
    int16_t range = p->cfg.max - p->cfg.min;
    if (range > 0) {
        int16_t max_fill = bar_w - 4;
        int16_t targ = (int16_t)((int32_t)(*p->cfg.value - p->cfg.min)
                                 * max_fill / range);
        if (targ > max_fill) targ = max_fill;

        /* 首次或目标变化时启动动画 */
        if (targ != p->bar_target) {
            int16_t start_w = p->bar_anim.cur_x;
            if (p->bar_target < 0) start_w = targ;   /* 首次跳转到位 */
            anim_start(&p->bar_anim, start_w, 0, targ, 0,
                       BAR_ANIM_MS, quad_ease_out);
            p->bar_target = targ;
        }

        int16_t fill_w = p->bar_anim.cur_x;
        if (fill_w > 0)
            u8g2_DrawBox(u8g2, bar_x + 2, bar_y + 2, fill_w, bar_h - 4);
    }

    /* 白色内边框 + 黑色外边框 (双层描边) */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawRFrame(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRFrame(u8g2, px-1, py-1, POPUP_W+2, POPUP_H+2, POPUP_R+1);
}


/* ================================================================
 *  开关弹窗
 *
 *  布局:
 *    标题(居中)
 *    直角矩形滑块开关 (仿 iOS/Android 风格)
 *  按键: Key1/2=翻转  Key3/4=关闭
 * ================================================================ */

static void popup_toggle_update(popup_toggle_t *p, int8_t key);
static void popup_toggle_render(popup_toggle_t *p, u8g2_t *u8g2);

static bool tgl_active(void *p)      { return ((popup_toggle_t *)p)->state != POPUP_IDLE; }
static void tgl_update(void *p, int8_t key) { popup_toggle_update((popup_toggle_t *)p, key); }
static void tgl_render(void *p, u8g2_t *u8g2) { popup_toggle_render((popup_toggle_t *)p, u8g2); }

void popup_toggle_init(popup_toggle_t *p, popup_base_t *b) {
    p->state = POPUP_IDLE;
    anim_init(&p->slide);
    anim_init(&p->knob_x);
    p->knob_target = -1;
    b->instance = p;
    b->active   = tgl_active;
    b->update   = tgl_update;
    b->render   = tgl_render;
    popup_mgr_register(b);
}

void popup_toggle_open(popup_toggle_t *p, const char *title, bool *value,
                       const char *text_on, const char *text_off) {
    p->cfg.title    = title;
    p->cfg.value    = value;
    p->cfg.text_on  = text_on;
    p->cfg.text_off = text_off;
    p->state        = POPUP_OPENING;
    p->knob_target  = -1;
    p->saved        = *value;           /* 保存原始值, Key4 取消时还原 */
    anim_start(&p->slide, 0, POPUP_OFF, 0, POPUP_Y, POPUP_OPEN_MS, quad_ease_out);
}

static void popup_toggle_update(popup_toggle_t *p, int8_t key) {
    switch (p->state) {
    case POPUP_OPENING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_ACTIVE;
        break;
    case POPUP_ACTIVE:
        if (key == 1 || key == 2) {      /* 上下键均翻转 */
            *p->cfg.value = !*p->cfg.value;
        } else if (key == 3) {
            /* 确认 — 保留修改 */
            anim_start(&p->slide, 0, p->slide.cur_y, 0, POPUP_OFF,
                       POPUP_CLOSE_MS, quad_ease_out);
            p->state = POPUP_CLOSING;
        } else if (key == 4) {
            /* 取消 — 还原原始值 */
            *p->cfg.value = p->saved;
            anim_start(&p->slide, 0, p->slide.cur_y, 0, POPUP_OFF,
                       POPUP_CLOSE_MS, quad_ease_out);
            p->state = POPUP_CLOSING;
        }
        break;
    case POPUP_CLOSING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_IDLE;
        break;
    default: break;
    }
}

static void popup_toggle_render(popup_toggle_t *p, u8g2_t *u8g2) {
    if (p->state == POPUP_IDLE) return;
    int16_t py = p->slide.cur_y;
    int16_t px = (128 - POPUP_W) / 2;

    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRBox(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);

    /* 标题 (居中) */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    u8g2_uint_t tt_w = u8g2_GetStrWidth(u8g2, p->cfg.title);
    u8g2_DrawStr(u8g2, px + (POPUP_W - (int16_t)tt_w) / 2, py + 14, p->cfg.title);

    /* 滑块开关 (直角矩形, 30x14 轨道 + 10x10 滑块, 动画驱动) */
    #define SW_W  30
    #define SW_H  14
    #define KNOB  10

    int16_t sw_x = px + (POPUP_W - SW_W) / 2;
    int16_t sw_y = py + 22;
    int16_t knob_off = sw_x + 2;             /* OFF 位置 */
    int16_t knob_on  = sw_x + SW_W - 2 - KNOB; /* ON 位置 */
    int16_t targ = *p->cfg.value ? knob_on : knob_off;

    /* 目标变化时启动动画 */
    if (targ != p->knob_target) {
        int16_t start_x = p->knob_x.cur_x;
        if (p->knob_target < 0) start_x = targ;  /* 首次跳转 */
        anim_start(&p->knob_x, start_x, 0, targ, 0,
                   BAR_ANIM_MS, quad_ease_out);
        p->knob_target = targ;
    }

    int16_t kx = p->knob_x.cur_x;             /* 当前动画 X */

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawFrame(u8g2, sw_x, sw_y, SW_W, SW_H);   /* 轨道外框 */

    /* 填充条: 从轨道左内边延伸到拨杆左侧, 留 2 px 黑缝 */
    int16_t fill_w = kx - sw_x - 4;           /* sw_x+2 到 kx, 中间 2px 缝 */
    if (fill_w > 0) {
        u8g2_DrawBox(u8g2, sw_x + 2, sw_y + 3, fill_w, 8);
    }
    /* 拨杆 */
    u8g2_DrawBox(u8g2, kx, sw_y + 2, KNOB, KNOB);

    #undef SW_W
    #undef SW_H
    #undef KNOB

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawRFrame(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRFrame(u8g2, px-1, py-1, POPUP_W+2, POPUP_H+2, POPUP_R+1);
}


/* ================================================================
 *  Toast 通知弹窗
 *
 *  紧凑尺寸 (宽度跟随文字, 高度紧凑)
 *  无交互 — 1 秒后自动消失
 *  update 不需要 key 参数 (由计时器驱动)
 * ================================================================ */

#define TOAST_PAD_X   8                    /* 水平内边距          */
#define TOAST_PAD_Y   5                    /* 垂直内边距          */
#define TOAST_R       3                    /* 圆角半径 (较小)     */
#define TOAST_H       18                   /* 预估高度, 仅用于居中计算 */
#define TOAST_Y       ((64 - TOAST_H) / 2) /* = 23 */
#define TOAST_OFF     (-TOAST_H)

static void popup_toast_update(popup_toast_t *p);
static void popup_toast_render(const popup_toast_t *p, u8g2_t *u8g2);

static bool tst_active(void *p)       { return ((popup_toast_t *)p)->state != POPUP_IDLE; }
static void tst_upd(void *p, int8_t key) { (void)key; popup_toast_update((popup_toast_t *)p); }
static void tst_rndr(void *p, u8g2_t *u8g2) { popup_toast_render((popup_toast_t *)p, u8g2); }

void popup_toast_init(popup_toast_t *p, popup_base_t *b) {
    p->state = POPUP_IDLE;
    anim_init(&p->slide);
    b->instance = p;
    b->active   = tst_active;
    b->update   = tst_upd;
    b->render   = tst_rndr;
    popup_mgr_register(b);
}

void popup_toast_show(popup_toast_t *p, const char *text) {
    p->text      = text;
    p->open_time = HAL_GetTick();
    p->state     = POPUP_OPENING;
    anim_start(&p->slide, 0, TOAST_OFF, 0, TOAST_Y, TOAST_ANIM_MS, quad_ease_out);
}

static void popup_toast_update(popup_toast_t *p) {
    switch (p->state) {
    case POPUP_OPENING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_ACTIVE;
        break;
    case POPUP_ACTIVE:
        /* 计时到后自动滑出 */
        if (HAL_GetTick() - p->open_time >= TOAST_DURATION) {
            anim_start(&p->slide, 0, p->slide.cur_y, 0, TOAST_OFF,
                       TOAST_ANIM_MS, quad_ease_out);
            p->state = POPUP_CLOSING;
        }
        break;
    case POPUP_CLOSING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_IDLE;
        break;
    default: break;
    }
}

static void popup_toast_render(const popup_toast_t *p, u8g2_t *u8g2) {
    if (p->state == POPUP_IDLE) return;

    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    int16_t     ascent = u8g2_GetAscent(u8g2);
    u8g2_uint_t str_w  = u8g2_GetStrWidth(u8g2, p->text);

    /* 尺寸跟随文字 */
    int16_t bw = (int16_t)str_w + TOAST_PAD_X * 2;
    int16_t bh = ascent + 3 + TOAST_PAD_Y * 2;
    int16_t bx = (128 - bw) / 2;
    int16_t by = p->slide.cur_y;

    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRBox(u8g2, bx, by, bw, bh, TOAST_R);

    u8g2_SetDrawColor(u8g2, 1);
    int16_t text_y = by + (bh + ascent) / 2;   /* 竖直居中 */
    u8g2_DrawStr(u8g2, bx + TOAST_PAD_X, text_y, p->text);

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawRFrame(u8g2, bx, by, bw, bh, TOAST_R);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRFrame(u8g2, bx-1, by-1, bw+2, bh+2, TOAST_R+1);
}
