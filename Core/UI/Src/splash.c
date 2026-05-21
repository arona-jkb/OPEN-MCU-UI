#include "splash.h"
#include "ui_timing.h"

/* 飞入/飞出起始 Y (屏幕上方) 及各元素目标 Y */
#define START_Y   (-15)
#define LINE1_Y   28                       /* "power by"    基线 */
#define LINE2_Y   45                       /* "OPEN MCU UI" 基线 */
#define BORDER_Y  63                       /* 底部分界线          */

void splash_init(splash_t *s) {
    s->state = SPLASH_ENTER;
    anim_init(&s->line1);
    anim_init(&s->line2);
    anim_init(&s->border);

    /* 三元素从屏幕上方同一位置飞入各自目标 */
    anim_start(&s->line1,  0, START_Y, 0, LINE1_Y,  SPLASH_ENTER_MS, quad_ease_out);
    anim_start(&s->line2,  0, START_Y, 0, LINE2_Y,  SPLASH_ENTER_MS, quad_ease_out);
    anim_start(&s->border, 0, START_Y, 0, BORDER_Y, SPLASH_ENTER_MS, quad_ease_out);
}

bool splash_done(const splash_t *s) {
    return s->state == SPLASH_DONE;
}

void splash_update(splash_t *s) {
    switch (s->state) {
    case SPLASH_ENTER:
        /* 等待所有飞入动画完成 */
        if ((s->line1.state == ANIM_FINISHED || s->line1.state == ANIM_IDLE) &&
            (s->line2.state == ANIM_FINISHED || s->line2.state == ANIM_IDLE)) {
            s->state      = SPLASH_HOLD;
            s->hold_start = HAL_GetTick();
        }
        break;

    case SPLASH_HOLD:
        /* 静止展示指定时长后启动飞出动画 */
        if (HAL_GetTick() - s->hold_start >= SPLASH_HOLD_MS) {
            anim_start(&s->line1,  0, s->line1.cur_y,  0, START_Y, SPLASH_EXIT_MS, quad_ease_out);
            anim_start(&s->line2,  0, s->line2.cur_y,  0, START_Y, SPLASH_EXIT_MS, quad_ease_out);
            anim_start(&s->border, 0, s->border.cur_y, 0, START_Y, SPLASH_EXIT_MS, quad_ease_out);
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
        /* 退出阶段: 只遮白线以上部分, 白线以下露出菜单 */
        if (by > 0) {
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawBox(u8g2, 0, 0, 128, by);
        }
    } else {
        /* 进入/静止阶段: 全屏黑底 */
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, 0, 0, 128, 64);
    }

    /* 白色分界线 (动画 Y) */
    if (by >= 0 && by < 64) {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawHLine(u8g2, 0, by, 128);
    }

    /* 第一行 — "power by" (小字) */
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    const char *l1 = "power by";
    u8g2_uint_t w1 = u8g2_GetStrWidth(u8g2, l1);
    int16_t y1 = s->line1.cur_y;
    if (y1 >= 0 && y1 < 64) {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawStr(u8g2, (128 - (int16_t)w1) / 2, y1, l1);
    }

    /* 第二行 — "OPEN MCU UI" (斜体大字) */
    u8g2_SetFont(u8g2, u8g2_font_luBIS10_tr);
    const char *l2 = "OPEN MCU UI";
    u8g2_uint_t w2 = u8g2_GetStrWidth(u8g2, l2);
    int16_t y2 = s->line2.cur_y;
    if (y2 >= 0 && y2 < 64) {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawStr(u8g2, (128 - (int16_t)w2) / 2, y2, l2);
    }
}

/* 一体化渲染: 清屏 → 背景 → 前景 → 发送 */
void splash_render_frame(const splash_t *s, u8g2_t *u8g2,
                         void (*bg_render)(void *ctx, u8g2_t *u8g2),
                         void *bg_ctx) {
    u8g2_ClearBuffer(u8g2);
    if (s->state == SPLASH_EXIT && bg_render) {
        bg_render(bg_ctx, u8g2);           /* 退出阶段: 菜单作为背景渐显 */
    }
    splash_render(s, u8g2);
    u8g2_SendBuffer(u8g2);
}
