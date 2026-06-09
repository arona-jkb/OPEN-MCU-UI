/**
 * 确认对话框
 *
 *  布局: 标题居中, 下划线动画指示焦点, 左右两个按钮
 *  按键: Key1/2 切换焦点  Key3=当前焦点项  Key4=取消
 *  结果通过 on_result 回调通知调用者 (true=确认, false=取消)
 */
#include "popup_confirm.h"
#include "anitime_config.h"

/* 共享布局常数 */
#define POPUP_W     110
#define POPUP_H      46
#define POPUP_R       4
#define POPUP_Y      ((64 - POPUP_H) / 2)
#define POPUP_OFF    (-POPUP_H)
#define ULINE_H     2

/* 按钮焦点 */
#define FOCUS_OK      0
#define FOCUS_CANCEL  1

static uint8_t g_focus = FOCUS_OK;

/* ---- 管理器回调包装 ---- */
static void popup_confirm_update(popup_confirm_t *p, int8_t key);
static void popup_confirm_render(popup_confirm_t *p, u8g2_t *u8g2);

static bool cfm_active(void *p)       { return ((popup_confirm_t *)p)->state != POPUP_IDLE; }
static void cfm_update(void *p, int8_t key) { popup_confirm_update((popup_confirm_t *)p, key); }
static void cfm_render(void *p, u8g2_t *u8g2) { popup_confirm_render((popup_confirm_t *)p, u8g2); }

void popup_confirm_init(popup_confirm_t *p, popup_base_t *b) {
    p->state = POPUP_IDLE;
    anim_init(&p->slide);
    anim_init(&p->uline_x);
    p->uline_x_targ = -1;
    p->uline_w_targ = -1;
    b->instance = p;
    b->active   = cfm_active;
    b->update   = cfm_update;
    b->render   = cfm_render;
    popup_mgr_register(b);
}

void popup_confirm_open(popup_confirm_t *p, const char *text,
                        confirm_callback_t on_result) {
    p->text         = text;
    p->on_result    = on_result;
    p->state        = POPUP_OPENING;
    g_focus         = FOCUS_OK;
    p->uline_x_targ = -1;
    p->uline_w_targ = -1;
    anim_start(&p->slide, 0, POPUP_OFF, 0, POPUP_Y, POPUP_OPEN_MS, quad_ease_out);
}

static void popup_confirm_update(popup_confirm_t *p, int8_t key) {
    switch (p->state) {
    case POPUP_OPENING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_ACTIVE;
        break;

    case POPUP_ACTIVE:
        if (key == 1 || key == 2) {
            g_focus = (g_focus == FOCUS_OK) ? FOCUS_CANCEL : FOCUS_OK;
        } else if (key == 3) {
            if (p->on_result) p->on_result(g_focus == FOCUS_OK);
            anim_start(&p->slide, 0, p->slide.cur_y, 0, POPUP_OFF,
                       POPUP_CLOSE_MS, quad_ease_out);
            p->state = POPUP_CLOSING;
        } else if (key == 4) {
            if (p->on_result) p->on_result(false);
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

static void popup_confirm_render(popup_confirm_t *p, u8g2_t *u8g2) {
    if (p->state == POPUP_IDLE) return;
    int16_t py = p->slide.cur_y;
    int16_t px = (128 - POPUP_W) / 2;

    /* 黑底遮罩 */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRBox(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);

    /* 提示文字 + OK/Cancel 按钮 (卡片边框内) */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    int16_t     ascent  = u8g2_GetAscent(u8g2);
    u8g2_uint_t text_w  = u8g2_GetStrWidth(u8g2, p->text);
    int16_t     text_y  = py + 14;
    u8g2_DrawStr(u8g2, px + (POPUP_W - (int16_t)text_w) / 2, text_y, p->text);

    /* 分隔线 */
    u8g2_DrawHLine(u8g2, px + 8, py + 22, POPUP_W - 16);

    /* 按钮布局 */
    int16_t btn_y        = py + ascent + 26;
    int16_t ok_cx        = px + POPUP_W / 4;
    int16_t cancel_cx    = px + (POPUP_W * 3) / 4;
    const char *lb_ok    = "OK";
    const char *lb_cancel = "Cancel";

    u8g2_uint_t ok_w     = u8g2_GetStrWidth(u8g2, lb_ok);
    u8g2_uint_t cancel_w = u8g2_GetStrWidth(u8g2, lb_cancel);
    int16_t     ok_x     = ok_cx     - (int16_t)(ok_w / 2);
    int16_t     cancel_x = cancel_cx - (int16_t)(cancel_w / 2);

    /* 绘制按钮文字 (不区分焦点, 统一颜色) */
    u8g2_DrawStr(u8g2, (u8g2_uint_t)ok_x,     btn_y, lb_ok);
    u8g2_DrawStr(u8g2, (u8g2_uint_t)cancel_x, btn_y, lb_cancel);

    /* ---- 下划线动画 (X=位置, Y=宽度) ---- */
    {
        int16_t targ_x, targ_w;
        if (g_focus == FOCUS_OK) {
            targ_x = ok_x;
            targ_w = (int16_t)ok_w;
        } else {
            targ_x = cancel_x;
            targ_w = (int16_t)cancel_w;
        }

        if (targ_x != p->uline_x_targ || targ_w != p->uline_w_targ) {
            int16_t start_x = p->uline_x.cur_x;
            int16_t start_w = p->uline_x.cur_y;
            if (p->uline_x_targ < 0) { start_x = targ_x; start_w = targ_w; }  /* 首帧定位 */
            anim_start(&p->uline_x, start_x, start_w,
                       targ_x, targ_w, BAR_ANIM_MS, quad_ease_out);
            p->uline_x_targ = targ_x;
            p->uline_w_targ = targ_w;
        }
    }

    /* 绘制下划线 (当前动画帧) */
    {
        int16_t ux = p->uline_x.cur_x;
        int16_t uw = p->uline_x.cur_y;
        if (uw > 0) {
            u8g2_SetDrawColor(u8g2, 1);
            u8g2_DrawBox(u8g2, (u8g2_uint_t)ux, (u8g2_uint_t)(btn_y + 2),
                         (u8g2_uint_t)uw, ULINE_H);
        }
    }

    /* 双层描边 */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawRFrame(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRFrame(u8g2, px-1, py-1, POPUP_W+2, POPUP_H+2, POPUP_R+1);
}
