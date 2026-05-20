#include "splash.h"

#define ENTER_MS  500
#define HOLD_MS   1000
#define EXIT_MS   400
#define START_Y   (-15)
#define LINE1_Y   28
#define LINE2_Y   45
#define BORDER_Y  63

void splash_init(splash_t *s) {
    s->state = SPLASH_ENTER;
    anim_init(&s->line1);
    anim_init(&s->line2);
    anim_init(&s->border);

    /* all three start from same Y above screen, fly to their targets */
    anim_start(&s->line1,  0, START_Y, 0, LINE1_Y,  ENTER_MS, quad_ease_out);
    anim_start(&s->line2,  0, START_Y, 0, LINE2_Y,  ENTER_MS, quad_ease_out);
    anim_start(&s->border, 0, START_Y, 0, BORDER_Y, ENTER_MS, quad_ease_out);
}

bool splash_done(const splash_t *s) {
    return s->state == SPLASH_DONE;
}

void splash_update(splash_t *s) {
    switch (s->state) {
    case SPLASH_ENTER:
        if ((s->line1.state == ANIM_FINISHED || s->line1.state == ANIM_IDLE) &&
            (s->line2.state == ANIM_FINISHED || s->line2.state == ANIM_IDLE)) {
            s->state      = SPLASH_HOLD;
            s->hold_start = HAL_GetTick();
        }
        break;

    case SPLASH_HOLD:
        if (HAL_GetTick() - s->hold_start >= HOLD_MS) {
            /* fly back up */
            anim_start(&s->line1,  0, s->line1.cur_y,  0, START_Y, EXIT_MS, quad_ease_out);
            anim_start(&s->line2,  0, s->line2.cur_y,  0, START_Y, EXIT_MS, quad_ease_out);
            anim_start(&s->border, 0, s->border.cur_y, 0, START_Y, EXIT_MS, quad_ease_out);
            s->state = SPLASH_EXIT;
        }
        break;

    case SPLASH_EXIT:
        if ((s->line1.state == ANIM_FINISHED || s->line1.state == ANIM_IDLE) &&
            (s->line2.state == ANIM_FINISHED || s->line2.state == ANIM_IDLE)) {
            s->state = SPLASH_DONE;
        }
        break;

    default: break;
    }
}

void splash_render(const splash_t *s, u8g2_t *u8g2) {
    if (s->state == SPLASH_DONE) return;

    int16_t by = s->border.cur_y;

    if (s->state == SPLASH_EXIT) {
        /* exit: black curtain above border, menu visible below */
        if (by > 0) {
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawBox(u8g2, 0, 0, 128, by);
        }
    } else {
        /* enter / hold: full-screen black */
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, 0, 0, 128, 64);
    }

    /* white border (animated) */
    if (by >= 0 && by < 64) {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawHLine(u8g2, 0, by, 128);
    }

    /* line 1 — "power by" */
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    const char *l1 = "power by";
    u8g2_uint_t w1 = u8g2_GetStrWidth(u8g2, l1);
    int16_t y1 = s->line1.cur_y;
    if (y1 >= 0 && y1 < 64) {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawStr(u8g2, (128 - (int16_t)w1) / 2, y1, l1);
    }

    /* line 2 — "OPEN MCU UI" */
    u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
    const char *l2 = "OPEN MCU UI";
    u8g2_uint_t w2 = u8g2_GetStrWidth(u8g2, l2);
    int16_t y2 = s->line2.cur_y;
    if (y2 >= 0 && y2 < 64) {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawStr(u8g2, (128 - (int16_t)w2) / 2, y2, l2);
    }
}
