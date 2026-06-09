/**
 * 仪表盘组件
 *
 * 标题栏 + 多项数值进度条, 实时显示传感器/参数数据。
 * 入场: 标题从顶部落下, 每行从右滑入 (stagger), 进度条从 0 展开。
 * 退场: 反向。
 */
#include "meter.h"
#include "ui_timing.h"
#include <stdio.h>

/* ---- 布局常数 ---- */
#define TITLE_H       12                   /* 标题栏高 (与 menu 保持一致) */
#define ROW_H         17                   /* 每行高度                */
#define METRIC_TOP    (TITLE_H + 2)        /* 首行内容 Y 起点 = 14    */
#define BAR_X         4
#define BAR_W         120                  /* 进度条外宽              */
#define LABEL_W       80                   /* 标签文字区域宽度        */

/* ---- 动画时长 ---- */
#define METER_ENTER_ROW_BASE  400          /* 首行飞入时长            */
#define METER_ENTER_ROW_STEP  120          /* 每行递增延迟            */
#define METER_ENTER_TITLE_MS  500          /* 标题入场时长            */
#define METER_ENTER_FILL_MS   600          /* 填充展开时长            */
#define METER_EXIT_MS         450          /* 退场时长                */

/* ---- 工具: 计算进度条填充宽度 (像素) ---- */
static int16_t calc_fill_w(const meter_item_t *item) {
    int16_t range = item->max - item->min;
    if (range <= 0) return 0;
    int16_t max_fill = BAR_W - 4;          /* 留 2px 边框内边距 */
    int16_t val = *item->value;
    if (val < item->min) val = item->min;
    if (val > item->max) val = item->max;
    return (int16_t)((int32_t)(val - item->min) * max_fill / range);
}

/* ================================================================
 *  生命周期
 * ================================================================ */

void meter_init(meter_state_t *s) {
    s->page  = NULL;
    s->trans = METER_IDLE;
    anim_init(&s->slide);
    anim_set_position(&s->slide, 0, 0);
    anim_init(&s->title_y);
    anim_set_position(&s->title_y, 0, TITLE_H - 1);
    for (int i = 0; i < METER_MAX_ITEMS; i++) {
        anim_init(&s->item_x[i]);
        anim_set_position(&s->item_x[i], 0, 0);
        anim_init(&s->bar_fill_w[i]);
        anim_set_position(&s->bar_fill_w[i], 0, 0);
        s->bar_fill_target[i] = -1;
    }
}

bool meter_active(const meter_state_t *s) {
    return s->trans != METER_IDLE;
}

void meter_open(meter_state_t *s, const meter_page_t *page) {
    s->page  = page;
    s->trans = METER_ENTER;

    /* 标题: 从上方飞入 */
    anim_start(&s->title_y, 0, -12, 0, TITLE_H - 1, METER_ENTER_TITLE_MS, quad_ease_out);

    /* 每行: 从右滑入 (stagger) */
    for (uint8_t i = 0; i < page->count && i < METER_MAX_ITEMS; i++) {
        int16_t row_y = METRIC_TOP + (int16_t)i * ROW_H;
        uint32_t dur = (uint32_t)METER_ENTER_ROW_BASE + (uint32_t)i * METER_ENTER_ROW_STEP;
        anim_start(&s->item_x[i], 128, row_y, 0, row_y, dur, quad_ease_out);
    }

    /* 进度条填充: 从 0 展开 */
    for (uint8_t i = 0; i < page->count && i < METER_MAX_ITEMS; i++) {
        int16_t targ = calc_fill_w(&page->items[i]);
        s->bar_fill_target[i] = targ;
        anim_start(&s->bar_fill_w[i], 0, 0, targ, 0, METER_ENTER_FILL_MS, quad_ease_out);
    }
}

void meter_close(meter_state_t *s) {
    if (!s->page) return;
    s->trans = METER_EXIT;

    /* 标题滑出到上方 */
    anim_start(&s->title_y, 0, s->title_y.cur_y, 0, -12, METER_EXIT_MS, quad_ease_out);

    /* 行滑回右侧 + 进度条收缩 */
    for (uint8_t i = 0; i < s->page->count && i < METER_MAX_ITEMS; i++) {
        anim_start(&s->item_x[i], s->item_x[i].cur_x, s->item_x[i].cur_y,
                   128, s->item_x[i].cur_y, METER_EXIT_MS, quad_ease_out);
        anim_start(&s->bar_fill_w[i], s->bar_fill_w[i].cur_x, 0,
                   0, 0, METER_EXIT_MS, quad_ease_out);
    }
}

void meter_update(meter_state_t *s) {
    if (s->trans == METER_ENTER) {
        /* 等最后一行飞入完成 */
        uint8_t n = s->page->count;
        if (n == 0) { s->trans = METER_ACTIVE; return; }
        uint8_t last = n - 1;
        if (last >= METER_MAX_ITEMS) last = METER_MAX_ITEMS - 1;
        if (s->item_x[last].state == ANIM_FINISHED || s->item_x[last].state == ANIM_IDLE)
            s->trans = METER_ACTIVE;

    } else if (s->trans == METER_EXIT) {
        /* 等最后一行滑出完成 → 回到 IDLE */
        uint8_t n = s->page->count;
        if (n == 0) { s->trans = METER_IDLE; return; }
        uint8_t last = n - 1;
        if (last >= METER_MAX_ITEMS) last = METER_MAX_ITEMS - 1;
        if (s->item_x[last].state == ANIM_FINISHED || s->item_x[last].state == ANIM_IDLE) {
            s->trans = METER_IDLE;
            s->page  = NULL;
        }
    }
}

/* ================================================================
 *  渲染
 * ================================================================ */

void meter_render(const meter_state_t *s, u8g2_t *u8g2) {
    if (s->trans == METER_IDLE || !s->page) return;

    const meter_page_t *page = s->page;
    u8g2_SetFontMode(u8g2, 1);

    /* ---- 标题栏 (黑底白字, 动画 Y) ---- */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawBox(u8g2, 0, 0, 128, TITLE_H);
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    {
        int16_t ty = s->title_y.cur_y;
        if (ty >= 0 && ty < 64) {
            u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, page->title);
            int16_t tx = (int16_t)(128 - tw) / 2; if (tx < 0) tx = 0;
            u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, ty - 3, page->title);
        }
    }

    /* ---- 每行仪表项 ---- */
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
    int16_t ascent = u8g2_GetAscent(u8g2);

    for (uint8_t i = 0; i < page->count && i < METER_MAX_ITEMS; i++) {
        const meter_item_t *item = &page->items[i];
        int16_t row_y = METRIC_TOP + (int16_t)i * ROW_H;
        int16_t cur_x = s->item_x[i].cur_x;

        /* 标签 (左对齐) */
        int16_t label_base = row_y + ascent;
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawStr(u8g2, (u8g2_uint_t)(cur_x + 4), label_base, item->label);

        /* 数值 + 单位 (右对齐, BAR_X + BAR_W = 124) */
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d %s", *item->value, item->unit);
            u8g2_uint_t vw = u8g2_GetStrWidth(u8g2, buf);
            int16_t vx = cur_x + BAR_X + BAR_W - (int16_t)vw - 2;
            if (vx > cur_x + LABEL_W)
                u8g2_DrawStr(u8g2, (u8g2_uint_t)vx, label_base, buf);
        }

        /* 进度条 (直角, y = 标签基线 + 3) */
        int16_t bar_w = s->bar_fill_w[i].cur_x;
        int16_t bar_y = label_base + 3;
        int16_t bar_h = item->bar_h;
        if (bar_h < 2) bar_h = 4;  /* 默认 4px 高 */

        /* 外框 */
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawFrame(u8g2, (u8g2_uint_t)(cur_x + BAR_X), bar_y,
                       (u8g2_uint_t)BAR_W, (u8g2_uint_t)bar_h);
        /* 填充 */
        if (bar_w > 0) {
            u8g2_DrawBox(u8g2, (u8g2_uint_t)(cur_x + BAR_X + 2), bar_y + 1,
                         (u8g2_uint_t)bar_w, (u8g2_uint_t)(bar_h - 2));
        }
    }
}
