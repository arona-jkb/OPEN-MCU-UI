#include "popup.h"
#include <stdio.h>

#define POPUP_W     110
#define POPUP_H      40
#define POPUP_R       4
#define POPUP_CENTER ((64 - POPUP_H) / 2)   /* = 12 */
#define POPUP_HIDDEN (-POPUP_H)

void popup_init(popup_t *p) {
    p->state = POPUP_IDLE;
    anim_init(&p->slide_anim);
}

void popup_open(popup_t *p, const char *title, int16_t *value,
                int16_t min, int16_t max, int16_t step) {
    p->cfg.title = title;
    p->cfg.value = value;
    p->cfg.min   = min;
    p->cfg.max   = max;
    p->cfg.step  = step;
    p->state     = POPUP_OPENING;
    anim_start(&p->slide_anim, 0, POPUP_HIDDEN, 0, POPUP_CENTER, 300, quad_ease_out);
}

bool popup_is_active(const popup_t *p) {
    return p->state != POPUP_IDLE;
}

void popup_update(popup_t *p, int8_t key) {
    switch (p->state) {
    case POPUP_OPENING:
        if (p->slide_anim.state == ANIM_FINISHED ||
            p->slide_anim.state == ANIM_IDLE) {
            p->state = POPUP_ACTIVE;
        }
        break;

    case POPUP_ACTIVE:
        if (key == 1) {
            *p->cfg.value += p->cfg.step;
            if (*p->cfg.value > p->cfg.max) *p->cfg.value = p->cfg.max;
        } else if (key == 2) {
            *p->cfg.value -= p->cfg.step;
            if (*p->cfg.value < p->cfg.min) *p->cfg.value = p->cfg.min;
        } else if (key == 3 || key == 4) {
            anim_start(&p->slide_anim, 0, p->slide_anim.cur_y, 0, POPUP_HIDDEN,
                       250, quad_ease_out);
            p->state = POPUP_CLOSING;
        }
        break;

    case POPUP_CLOSING:
        if (p->slide_anim.state == ANIM_FINISHED ||
            p->slide_anim.state == ANIM_IDLE) {
            p->state = POPUP_IDLE;
        }
        break;

    default:
        break;
    }
}

void popup_render(const popup_t *p, u8g2_t *u8g2) {
    if (p->state == POPUP_IDLE) return;

    int16_t py = p->slide_anim.cur_y;
    int16_t px = (128 - POPUP_W) / 2;

    /* --- popup body --- */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawRBox(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawRFrame(u8g2, px, py, POPUP_W, POPUP_H, POPUP_R);

    /* --- title --- */
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    u8g2_DrawStr(u8g2, px + 5, py + 10, p->cfg.title);

    /* --- horizontal rule --- */
    u8g2_DrawHLine(u8g2, px + 4, py + 13, POPUP_W - 8);

    /* --- value (centered) --- */
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", *p->cfg.value);
    u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
    u8g2_uint_t w = u8g2_GetStrWidth(u8g2, buf);
    u8g2_DrawStr(u8g2, px + (POPUP_W - w) / 2, py + 33, buf);

    u8g2_SetDrawColor(u8g2, 1);
}
