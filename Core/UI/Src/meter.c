/**
 * 仪表盘组件 — 两种类型:
 *   meter_bar  : 标题栏 + 多项数值进度条, 实时显示传感器/参数数据。
 *                入场: 标题从顶部落下, 每行从右滑入 (stagger), 进度条从 0 展开。
 *                退场: 反向。
 *   meter_quad : 四象限参数数值展示, 无进度条。
 *                布局: 2x2 网格, 参数名在上, 大字数值居中, 单位角标。
 *                入场: 标题从顶部落下, 各象限从下方滑入 (stagger)。
 *                退场: 反向。
 */
#include "meter.h"
#include "anitime_config.h"
#include <stdio.h>

/* ================================================================
 *  meter_bar — 进度条型仪表盘
 * ================================================================ */

/* ---- 布局常数 ---- */
#define BAR_TITLE_H       12                   /* 标题栏高 (与 menu 保持一致) */
#define BAR_ROW_H         17                   /* 每行高度                */
#define BAR_METRIC_TOP    (BAR_TITLE_H + 2)    /* 首行内容 Y 起点 = 14    */
#define BAR_X             4
#define BAR_W             120                  /* 进度条外宽              */
#define BAR_LABEL_W       80                   /* 标签文字区域宽度        */

/* ---- 动画时长 ---- */
#define BAR_ENTER_ROW_BASE  400                /* 首行飞入时长            */
#define BAR_ENTER_ROW_STEP  120                /* 每行递增延迟            */
#define BAR_ENTER_TITLE_MS  500                /* 标题入场时长            */
#define BAR_ENTER_FILL_MS   600                /* 填充展开时长            */
#define BAR_EXIT_MS         450                /* 退场时长                */

/* ---- 工具: 计算进度条填充宽度 (像素) ---- */
static int16_t bar_calc_fill_w(const meter_bar_item_t *item) {
    int16_t range = item->max - item->min;
    if (range <= 0) return 0;
    int16_t max_fill = BAR_W - 4;              /* 留 2px 边框内边距 */
    int16_t val = *item->value;
    if (val < item->min) val = item->min;
    if (val > item->max) val = item->max;
    return (int16_t)((int32_t)(val - item->min) * max_fill / range);
}

/* ================================================================
 *  meter_bar 生命周期
 * ================================================================ */

void meter_bar_init(meter_bar_state_t *s) {
    s->page  = NULL;
    s->trans = METER_BAR_IDLE;
    anim_init(&s->slide);
    anim_set_position(&s->slide, 0, 0);
    anim_init(&s->title_y);
    anim_set_position(&s->title_y, 0, BAR_TITLE_H - 1);
    for (int i = 0; i < METER_BAR_MAX_ITEMS; i++) {
        anim_init(&s->item_x[i]);
        anim_set_position(&s->item_x[i], 0, 0);
        anim_init(&s->bar_fill_w[i]);
        anim_set_position(&s->bar_fill_w[i], 0, 0);
        s->bar_fill_target[i] = -1;
    }
}

bool meter_bar_active(const meter_bar_state_t *s) {
    return s->trans != METER_BAR_IDLE;
}

void meter_bar_open(meter_bar_state_t *s, const meter_bar_page_t *page) {
    s->page  = page;
    s->trans = METER_BAR_ENTER;

    /* 标题: 从上方飞入 */
    anim_start(&s->title_y, 0, -12, 0, BAR_TITLE_H - 1, BAR_ENTER_TITLE_MS, quad_ease_out);

    /* 每行: 从右滑入 (stagger) */
    for (uint8_t i = 0; i < page->count && i < METER_BAR_MAX_ITEMS; i++) {
        int16_t row_y = BAR_METRIC_TOP + (int16_t)i * BAR_ROW_H;
        uint32_t dur = (uint32_t)BAR_ENTER_ROW_BASE + (uint32_t)i * BAR_ENTER_ROW_STEP;
        anim_start(&s->item_x[i], 128, row_y, 0, row_y, dur, quad_ease_out);
    }

    /* 进度条填充: 从 0 展开 */
    for (uint8_t i = 0; i < page->count && i < METER_BAR_MAX_ITEMS; i++) {
        int16_t targ = bar_calc_fill_w(&page->items[i]);
        s->bar_fill_target[i] = targ;
        anim_start(&s->bar_fill_w[i], 0, 0, targ, 0, BAR_ENTER_FILL_MS, quad_ease_out);
    }
}

void meter_bar_close(meter_bar_state_t *s) {
    if (!s->page) return;
    s->trans = METER_BAR_EXIT;

    /* 标题滑出到上方 */
    anim_start(&s->title_y, 0, s->title_y.cur_y, 0, -12, BAR_EXIT_MS, quad_ease_out);

    /* 行滑回右侧 + 进度条收缩 */
    for (uint8_t i = 0; i < s->page->count && i < METER_BAR_MAX_ITEMS; i++) {
        anim_start(&s->item_x[i], s->item_x[i].cur_x, s->item_x[i].cur_y,
                   128, s->item_x[i].cur_y, BAR_EXIT_MS, quad_ease_out);
        anim_start(&s->bar_fill_w[i], s->bar_fill_w[i].cur_x, 0,
                   0, 0, BAR_EXIT_MS, quad_ease_out);
    }
}

void meter_bar_update(meter_bar_state_t *s) {
    if (s->trans == METER_BAR_ENTER) {
        /* 等最后一行飞入完成 */
        uint8_t n = s->page->count;
        if (n == 0) { s->trans = METER_BAR_ACTIVE; return; }
        uint8_t last = n - 1;
        if (last >= METER_BAR_MAX_ITEMS) last = METER_BAR_MAX_ITEMS - 1;
        if (s->item_x[last].state == ANIM_FINISHED || s->item_x[last].state == ANIM_IDLE)
            s->trans = METER_BAR_ACTIVE;

    } else if (s->trans == METER_BAR_EXIT) {
        /* 等最后一行滑出完成 → 回到 IDLE */
        uint8_t n = s->page->count;
        if (n == 0) { s->trans = METER_BAR_IDLE; return; }
        uint8_t last = n - 1;
        if (last >= METER_BAR_MAX_ITEMS) last = METER_BAR_MAX_ITEMS - 1;
        if (s->item_x[last].state == ANIM_FINISHED || s->item_x[last].state == ANIM_IDLE) {
            s->trans = METER_BAR_IDLE;
            s->page  = NULL;
        }
    }
}

/* ================================================================
 *  meter_bar 渲染
 * ================================================================ */

void meter_bar_render(const meter_bar_state_t *s, u8g2_t *u8g2) {
    if (s->trans == METER_BAR_IDLE || !s->page) return;

    const meter_bar_page_t *page = s->page;
    u8g2_SetFontMode(u8g2, 1);

    /* ---- 标题栏 (黑底白字, 动画 Y) ---- */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawBox(u8g2, 0, 0, 128, BAR_TITLE_H);
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

    for (uint8_t i = 0; i < page->count && i < METER_BAR_MAX_ITEMS; i++) {
        const meter_bar_item_t *item = &page->items[i];
        int16_t row_y = BAR_METRIC_TOP + (int16_t)i * BAR_ROW_H;
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
            if (vx > cur_x + BAR_LABEL_W)
                u8g2_DrawStr(u8g2, (u8g2_uint_t)vx, label_base, buf);
        }

        /* 进度条 (直角, y = 标签基线 + 3) */
        int16_t bar_w = s->bar_fill_w[i].cur_x;
        int16_t bar_y = label_base + 3;
        int16_t bar_h = item->bar_h;
        if (bar_h < 2) bar_h = 4;             /* 默认 4px 高 */

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


/* ================================================================
 *  meter_quad — 四象限型仪表盘
 *
 *  屏幕布局 (128x64 OLED):
 *    ┌─────────────┬──────────────┐
 *    │  Title Bar  │  Title Bar   │  ← 12px 高
 *    ├─────────────┼──────────────┤
 *    │    Q1       │     Q2       │
 *    │  label      │   label      │  ← 25px 高
 *    │   **25**°C  │   **67**%    │
 *    ├─────────────┼──────────────┤
 *    │    Q3       │     Q4       │
 *    │  label      │   label      │  ← 25px 高
 *    │  **330**mV  │ **1013**hPa  │
 *    └─────────────┴──────────────┘
 * ================================================================ */

/* ---- 布局常数 ---- */
#define QUAD_TITLE_H     12
#define QUAD_CONTENT_Y   (QUAD_TITLE_H + 1)    /* 13 */
#define QUAD_CELL_W      63                     /* 每格宽 (留 1px 给分隔线或间距) */
#define QUAD_CELL_H      25                     /* 每格高 */
#define QUAD_COL2_X      65                     /* 第二列 X 起点 (64+1) */

/* ---- 动画时长 ---- */
#define QUAD_ENTER_TITLE_MS   400               /* 标题入场时长            */
#define QUAD_ENTER_ITEM_MS    350               /* 象限入场基础时长        */
#define QUAD_ENTER_STEP_MS    80                /* 象限入场递增延迟        */
#define QUAD_EXIT_MS          300               /* 退场时长                */

/* ---- 工具: 计算每个象限格的左上角坐标 ---- */
static void quad_cell_pos(uint8_t idx, int16_t *cx, int16_t *cy) {
    /* idx: 0=左上, 1=右上, 2=左下, 3=右下 */
    *cx = (idx & 1) ? QUAD_COL2_X : 1;
    *cy = QUAD_CONTENT_Y + ((idx & 2) ? QUAD_CELL_H : 0);
}

/* ================================================================
 *  meter_quad 生命周期
 * ================================================================ */

void meter_quad_init(meter_quad_state_t *s) {
    s->page  = NULL;
    s->trans = METER_QUAD_IDLE;
    anim_init(&s->title_y);
    anim_set_position(&s->title_y, 0, QUAD_TITLE_H - 1);
    for (int i = 0; i < METER_QUAD_MAX_ITEMS; i++) {
        anim_init(&s->item_y[i]);
        anim_set_position(&s->item_y[i], 0, 0);
        s->cell_x[i] = 0;
    }
}

bool meter_quad_active(const meter_quad_state_t *s) {
    return s->trans != METER_QUAD_IDLE;
}

void meter_quad_open(meter_quad_state_t *s, const meter_quad_page_t *page) {
    s->page  = page;
    s->trans = METER_QUAD_ENTER;

    /* 标题: 从上方飞入 */
    anim_start(&s->title_y, 0, -12, 0, QUAD_TITLE_H - 1,
               QUAD_ENTER_TITLE_MS, quad_ease_out);

    /* 每象限: 从下方滑入 (stagger) */
    for (uint8_t i = 0; i < page->count && i < METER_QUAD_MAX_ITEMS; i++) {
        int16_t cx, cy;
        quad_cell_pos(i, &cx, &cy);
        s->cell_x[i] = cx;
        uint32_t dur = (uint32_t)QUAD_ENTER_ITEM_MS + (uint32_t)i * QUAD_ENTER_STEP_MS;
        /* 从目标位置下方 12px 滑入 */
        anim_start(&s->item_y[i], 0, cy + 12, 0, cy, dur, quad_ease_out);
    }
}

void meter_quad_close(meter_quad_state_t *s) {
    if (!s->page) return;
    s->trans = METER_QUAD_EXIT;

    /* 标题滑出 */
    anim_start(&s->title_y, 0, s->title_y.cur_y, 0, -12,
               QUAD_EXIT_MS, quad_ease_out);

    /* 象限向下滑出 */
    for (uint8_t i = 0; i < s->page->count && i < METER_QUAD_MAX_ITEMS; i++) {
        anim_start(&s->item_y[i], 0, s->item_y[i].cur_y,
                   0, s->item_y[i].cur_y + 12, QUAD_EXIT_MS, quad_ease_out);
    }
}

void meter_quad_update(meter_quad_state_t *s) {
    if (s->trans == METER_QUAD_ENTER) {
        uint8_t n = s->page->count;
        if (n == 0) { s->trans = METER_QUAD_ACTIVE; return; }
        uint8_t last = n - 1;
        if (last >= METER_QUAD_MAX_ITEMS) last = METER_QUAD_MAX_ITEMS - 1;
        if (s->item_y[last].state == ANIM_FINISHED ||
            s->item_y[last].state == ANIM_IDLE)
            s->trans = METER_QUAD_ACTIVE;

    } else if (s->trans == METER_QUAD_EXIT) {
        uint8_t n = s->page->count;
        if (n == 0) { s->trans = METER_QUAD_IDLE; return; }
        uint8_t last = n - 1;
        if (last >= METER_QUAD_MAX_ITEMS) last = METER_QUAD_MAX_ITEMS - 1;
        if (s->item_y[last].state == ANIM_FINISHED ||
            s->item_y[last].state == ANIM_IDLE) {
            s->trans = METER_QUAD_IDLE;
            s->page  = NULL;
        }
    }
}

/* ================================================================
 *  meter_quad 渲染
 * ================================================================ */

void meter_quad_render(const meter_quad_state_t *s, u8g2_t *u8g2) {
    if (s->trans == METER_QUAD_IDLE || !s->page) return;

    const meter_quad_page_t *page = s->page;
    u8g2_SetFontMode(u8g2, 1);

    /* ---- 标题栏 (黑底白字, 与 menu/bar 风格统一) ---- */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawBox(u8g2, 0, 0, 128, QUAD_TITLE_H);
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    {
        int16_t ty = s->title_y.cur_y;
        if (ty >= 0 && ty < 64) {
            u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, page->title);
            int16_t tx = (int16_t)(128 - tw) / 2;
            if (tx < 0) tx = 0;
            u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, ty - 3, page->title);
        }
    }

    /* ---- 四个象限 ---- */
    for (uint8_t i = 0; i < page->count && i < METER_QUAD_MAX_ITEMS; i++) {
        const meter_quad_item_t *item = &page->items[i];
        int16_t cx = s->cell_x[i];
        int16_t cy = s->item_y[i].cur_y;   /* 动画后的格子顶部 Y */

        /*
         * 格子内布局 (QUAD_CELL_W x QUAD_CELL_H = 63 x 25):
         *   ┌──────────┐
         *   │  label   │  ← helvB08, Y = cell_top + 8
         *   │  **25**°C│  ← helvB14 (数值) + 6x10 (单位下标)
         *   └──────────┘    数值 Y = cell_top + 22, 单位 Y = cell_top + 23
         */

        /* 参数名称 — 顶部居中 (小字) */
        u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
        u8g2_SetDrawColor(u8g2, 1);
        {
            u8g2_uint_t lw = u8g2_GetStrWidth(u8g2, item->label);
            int16_t lx = cx + (int16_t)(QUAD_CELL_W - lw) / 2;
            if (lx < cx) lx = cx;
            u8g2_DrawStr(u8g2, (u8g2_uint_t)lx, cy + 8, item->label);
        }

        /* 数值 (大字粗体 14px) + 单位 (小字下标 10px) */
        {
            char vbuf[8];
            snprintf(vbuf, sizeof(vbuf), "%d", *item->value);

            /* 测量宽度以实现整体居中 */
            u8g2_SetFont(u8g2, u8g2_font_helvB14_tr);
            u8g2_uint_t vw = u8g2_GetStrWidth(u8g2, vbuf);
            u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
            u8g2_uint_t uw = item->unit ? u8g2_GetStrWidth(u8g2, item->unit) : 0;
            u8g2_uint_t gap = uw ? 2 : 0;
            u8g2_uint_t total_w = vw + gap + uw;

            int16_t gx = cx + (int16_t)(QUAD_CELL_W - total_w) / 2;
            if (gx < cx) gx = cx;

            /* 数值 (大字) */
            u8g2_SetFont(u8g2, u8g2_font_helvB14_tr);
            u8g2_DrawStr(u8g2, (u8g2_uint_t)gx, cy + 22, vbuf);

            /* 单位 (小字, 基线偏移 +1px 产生角标效果) */
            if (item->unit && uw > 0) {
                u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
                u8g2_DrawStr(u8g2, (u8g2_uint_t)(gx + vw + gap), cy + 22, item->unit);
            }
        }
    }
}
