#include "popup.h"
#include <stdio.h>

/* ========== shared constants ========== */
#define POPUP_W     110
#define POPUP_H      40
#define POPUP_R       4
#define POPUP_Y      ((64 - POPUP_H) / 2)   /* = 12 */
#define POPUP_OFF    (-POPUP_H)

/* ========== numeric popup ========== */

void popup_num_init(popup_num_t *p) {
    p->state = POPUP_IDLE;
    anim_init(&p->slide);
}

void popup_num_open(popup_num_t *p, const char *title, int16_t *value,
                    int16_t min, int16_t max, int16_t step) {
    p->cfg.title = title;
    p->cfg.value = value;
    p->cfg.min   = min;
    p->cfg.max   = max;
    p->cfg.step  = step;
    p->state     = POPUP_OPENING;
    anim_start(&p->slide, 0, POPUP_OFF, 0, POPUP_Y, 300, quad_ease_out);
}

bool popup_num_active(const popup_num_t *p) {
    return p->state != POPUP_IDLE;
}

void popup_num_update(popup_num_t *p, int8_t key) {
    switch (p->state) {
    case POPUP_OPENING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_ACTIVE;
        break;
    case POPUP_ACTIVE:
        if (key == 1) {
            *p->cfg.value += p->cfg.step;
            if (*p->cfg.value > p->cfg.max) *p->cfg.value = p->cfg.max;
        } else if (key == 2) {
            *p->cfg.value -= p->cfg.step;
            if (*p->cfg.value < p->cfg.min) *p->cfg.value = p->cfg.min;
        } else if (key == 3 || key == 4) {
            anim_start(&p->slide, 0, p->slide.cur_y, 0, POPUP_OFF, 250, quad_ease_out);
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

void popup_num_render(const popup_num_t *p, u8g2_t *u8g2) {
    if (p->state == POPUP_IDLE) return;
    int16_t py = p->slide.cur_y;
    int16_t px = (128 - POPUP_W) / 2;

    /* black fill */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRBox(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);

    /* title (top-left) + value (top-right, ~3/4 across) */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    int16_t ttl_base = py + 13;
    u8g2_DrawStr(u8g2, px + 6, ttl_base, p->cfg.title);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", *p->cfg.value);
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    u8g2_uint_t w = u8g2_GetStrWidth(u8g2, buf);
    u8g2_DrawStr(u8g2, px + (POPUP_W * 3) / 4 - (int16_t)w / 2, ttl_base, buf);

    /* progress bar (straight frame, fill inset by 2 px) */
    int16_t bar_x  = px + 6;
    int16_t bar_w  = POPUP_W - 12;
    int16_t bar_h  = 8;
    int16_t bar_y  = py + 22;

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawFrame(u8g2, bar_x, bar_y, bar_w, bar_h);

    int16_t range = p->cfg.max - p->cfg.min;
    if (range > 0) {
        int16_t max_fill = bar_w - 4;
        int16_t fill_w = (int16_t)((int32_t)(*p->cfg.value - p->cfg.min)
                                   * max_fill / range);
        if (fill_w > max_fill) fill_w = max_fill;
        if (fill_w > 0)
            u8g2_DrawBox(u8g2, bar_x + 2, bar_y + 2, fill_w, bar_h - 4);
    }

    /* white inner border + black outer border */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawRFrame(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRFrame(u8g2, px-1, py-1, POPUP_W+2, POPUP_H+2, POPUP_R+1);
}


/* ========== boolean popup ========== */

void popup_bool_init(popup_bool_t *p) {
    p->state = POPUP_IDLE;
    anim_init(&p->slide);
}

void popup_bool_open(popup_bool_t *p, const char *title, bool *value,
                     const char *text_on, const char *text_off) {
    p->cfg.title    = title;
    p->cfg.value    = value;
    p->cfg.text_on  = text_on;
    p->cfg.text_off = text_off;
    p->state        = POPUP_OPENING;
    anim_start(&p->slide, 0, POPUP_OFF, 0, POPUP_Y, 300, quad_ease_out);
}

bool popup_bool_active(const popup_bool_t *p) {
    return p->state != POPUP_IDLE;
}

void popup_bool_update(popup_bool_t *p, int8_t key) {
    switch (p->state) {
    case POPUP_OPENING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_ACTIVE;
        break;
    case POPUP_ACTIVE:
        if (key == 1 || key == 2) {
            *p->cfg.value = !*p->cfg.value;
        } else if (key == 3 || key == 4) {
            anim_start(&p->slide, 0, p->slide.cur_y, 0, POPUP_OFF, 250, quad_ease_out);
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

void popup_bool_render(const popup_bool_t *p, u8g2_t *u8g2) {
    if (p->state == POPUP_IDLE) return;
    int16_t py = p->slide.cur_y;
    int16_t px = (128 - POPUP_W) / 2;

    /* black fill */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRBox(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);

    /* title (centred, small font) */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    u8g2_uint_t tt_w = u8g2_GetStrWidth(u8g2, p->cfg.title);
    u8g2_DrawStr(u8g2, px + (POPUP_W - (int16_t)tt_w) / 2, py + 14, p->cfg.title);

    /* toggle switch (straight rectangles) */
    #define SW_W  30
    #define SW_H  14
    #define KNOB   10

    int16_t sw_x = px + (POPUP_W - SW_W) / 2;
    int16_t sw_y = py + 22;

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawFrame(u8g2, sw_x, sw_y, SW_W, SW_H);

    if (*p->cfg.value) {
        /* ON: filled bar (2 px inside frame) + knob on right */
        u8g2_DrawBox(u8g2, sw_x + 2, sw_y + 3, 14, 8);
        u8g2_DrawBox(u8g2, sw_x + 18, sw_y + 2, KNOB, KNOB);
    } else {
        /* OFF: knob on left */
        u8g2_DrawBox(u8g2, sw_x + 2, sw_y + 2, KNOB, KNOB);
    }

    #undef SW_W
    #undef SW_H
    #undef KNOB

    /* white inner border + black outer border */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawRFrame(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRFrame(u8g2, px-1, py-1, POPUP_W+2, POPUP_H+2, POPUP_R+1);
}


/* ========== toast popup ========== */

#define TOAST_PAD_X   8
#define TOAST_PAD_Y   5
#define TOAST_R       3
#define TOAST_MS      1000
#define TOAST_ANIM_MS 180
#define TOAST_H       18
#define TOAST_Y       ((64 - TOAST_H) / 2)   /* = 23 */
#define TOAST_OFF     (-TOAST_H)

void popup_toast_init(popup_toast_t *p) {
    p->state = POPUP_IDLE;
    anim_init(&p->slide);
}

void popup_toast_show(popup_toast_t *p, const char *text) {
    p->text      = text;
    p->open_time = HAL_GetTick();
    p->state     = POPUP_OPENING;
    anim_start(&p->slide, 0, TOAST_OFF, 0, TOAST_Y, TOAST_ANIM_MS, quad_ease_out);
}

bool popup_toast_active(const popup_toast_t *p) {
    return p->state != POPUP_IDLE;
}

void popup_toast_update(popup_toast_t *p) {
    switch (p->state) {
    case POPUP_OPENING:
        if (p->slide.state == ANIM_FINISHED || p->slide.state == ANIM_IDLE)
            p->state = POPUP_ACTIVE;
        break;
    case POPUP_ACTIVE:
        if (HAL_GetTick() - p->open_time >= TOAST_MS) {
            anim_start(&p->slide, 0, p->slide.cur_y, 0, TOAST_OFF, TOAST_ANIM_MS, quad_ease_out);
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

void popup_toast_render(const popup_toast_t *p, u8g2_t *u8g2) {
    if (p->state == POPUP_IDLE) return;

    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    int16_t     ascent = u8g2_GetAscent(u8g2);
    u8g2_uint_t str_w  = u8g2_GetStrWidth(u8g2, p->text);

    int16_t bw = (int16_t)str_w + TOAST_PAD_X * 2;
    int16_t bh = ascent + 3 + TOAST_PAD_Y * 2;
    int16_t bx = (128 - bw) / 2;
    int16_t by = p->slide.cur_y;

    /* black fill */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRBox(u8g2, bx, by, bw, bh, TOAST_R);

    /* content (white on black) */
    u8g2_SetDrawColor(u8g2, 1);
    int16_t text_y = by + (bh + ascent) / 2;
    u8g2_DrawStr(u8g2, bx + TOAST_PAD_X, text_y, p->text);

    /* white inner border + black outer border */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawRFrame(u8g2, bx, by, bw, bh, TOAST_R);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRFrame(u8g2, bx-1, by-1, bw+2, bh+2, TOAST_R+1);
}
