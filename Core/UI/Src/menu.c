/**
 * 菜单模块实现
 *
 * 核心功能:
 *   - 多级层次菜单, 支持页面切换动画
 *   - 边界触发式滚动 (选中项碰顶/触底时整体文字才滚动)
 *   - 选择条独立动画 (位置 + 宽度, 由 bar_anim 驱动)
 *   - 右侧滚动进度指示条 (由 prog_anim 驱动)
 *   - XOR 模式选择框 (颜色 2), 与菜单文字自然叠加
 */
#include "menu.h"
#include "ui_timing.h"

#define BOX_PAD_X    5                    /* 选择框左右内边距      */
#define BOX_PAD_Y    1                    /* 选择框上下内边距      */
#define BOX_RADIUS   1                   /* 选择框圆角半径        */

#define BAR_MAX_W      122               /* 选择条最大宽度: 比右侧进度条(x=125)小1px */
#define TEXT_START_X   (4 + BOX_PAD_X)    /* 菜单文字起始 X 坐标 = 9                    */
#define TEXT_MAX_END   (2 + BAR_MAX_W - BOX_PAD_X) /* 文字可见右边界 = 119           */
#define TEXT_VISIBLE_W (TEXT_MAX_END - TEXT_START_X) /* 可见文字宽度 = 110            */

#define VISIBLE_TOP    MENU_TITLE_HEIGHT
#define VISIBLE_BOTTOM (64 - MENU_LINE_HEIGHT)

static int16_t calc_scroll_target(const menu_state_t *state) {
    uint8_t n = state->current->count;
    if (n == 0) return 0;
    int16_t cur = state->scroll_anim.cur_y;
    int16_t item_y = VISIBLE_TOP + (int16_t)state->selected * MENU_LINE_HEIGHT - cur;
    if (item_y < VISIBLE_TOP) {
        int16_t t = (int16_t)state->selected * MENU_LINE_HEIGHT;
        return t < 0 ? 0 : t;
    }
    if (item_y > VISIBLE_BOTTOM) {
        int16_t max_scroll = VISIBLE_TOP + (int16_t)(n - 1) * MENU_LINE_HEIGHT - VISIBLE_BOTTOM;
        if (max_scroll < 0) max_scroll = 0;
        int16_t t = VISIBLE_TOP + (int16_t)state->selected * MENU_LINE_HEIGHT - VISIBLE_BOTTOM;
        if (t > max_scroll) t = max_scroll;
        return t < 0 ? 0 : t;
    }
    return cur;
}

/* 前向声明 — 页面切换函数 */
static void trans_start_old(menu_state_t *state, int16_t ascent);
static void trans_start_new(menu_state_t *state, int16_t ascent);

static void start_scroll(menu_state_t *state) {
    int16_t target = calc_scroll_target(state);
    state->scroll_target = target;
    anim_start(&state->scroll_anim,
               0, state->scroll_anim.cur_y,
               0, target,
               SCROLL_ANIM_MS, quad_ease_out);

    /* 选中项变化时, 复位文字水平滚动 */
    state->text_scroll_target = -1;
    anim_stop(&state->text_scroll_anim);
    anim_set_position(&state->text_scroll_anim, 0, 0);
}

void menu_init(menu_state_t *state, const menu_page_t *root) {
    state->current = root;
    state->selected = 0;
    state->scroll_target = 0;
    anim_init(&state->scroll_anim);
    anim_set_position(&state->scroll_anim, 0, 0);
    anim_init(&state->text_scroll_anim);
    anim_set_position(&state->text_scroll_anim, 0, 0);
    state->text_scroll_target = -1;
    anim_init(&state->bar_anim);
    state->bar_target_y = -1;
    state->bar_target_w = -1;
    anim_init(&state->prog_anim);
    state->prog_target = -1;
    state->trans = TRANS_NONE;
    anim_init(&state->title_old);
    anim_init(&state->title_new);
    for (int i = 0; i < MENU_MAX_ITEMS; i++) {
        anim_init(&state->items_old[i]);
        anim_init(&state->items_new[i]);
    }
}

void menu_update(menu_state_t *state) {
    if (state->trans == TRANS_NONE) return;

    if (state->trans == TRANS_OLD_OUT) {
        /* 旧页退出完成 → 启动新页进入 */
        if (state->title_old.state == ANIM_FINISHED ||
            state->title_old.state == ANIM_IDLE) {
            state->trans = TRANS_NEW_IN;
            trans_start_new(state, 8);
        }
    } else if (state->trans == TRANS_NEW_IN) {
        /* 新页进入完成 → 结束过渡 */
        if (state->title_new.state == ANIM_FINISHED ||
            state->title_new.state == ANIM_IDLE) {
            state->trans = TRANS_NONE;
            anim_set_position(&state->scroll_anim, 0, 0);
            state->scroll_target = 0;
            state->bar_target_y = -1;
            state->bar_target_w = -1;
            state->text_scroll_target = -1;
            state->prog_target  = -1;
        }
    }
}

/* ======== main render ======== */

void menu_render(u8g2_t *u8g2, menu_state_t *state) {
    u8g2_SetFontMode(u8g2, 1);

    if (state->trans == TRANS_OLD_OUT) {
        /* ---- 旧页退出 (全部向上收缩至 y=-12) ---- */
        const menu_page_t *oldp = state->trans_old;
        int16_t ttl = VISIBLE_TOP;

        /* 旧页项 */
        u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
        for (uint8_t i = 0; i < oldp->count && i < MENU_MAX_ITEMS; i++) {
            int16_t y = state->items_old[i].cur_y;
            if (y < -10 || y > 65) continue;
            u8g2_SetDrawColor(u8g2, 1);
            u8g2_DrawStr(u8g2, 4 + BOX_PAD_X, y, oldp->items[i].name);
        }

        /* 旧页标题栏 (黑底白字) */
        {
            int16_t oty = state->title_old.cur_y;
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawBox(u8g2, 0, oty - ttl + 1, 128, ttl);
            u8g2_SetDrawColor(u8g2, 1);
            u8g2_DrawHLine(u8g2, 0, oty, 128);
            u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
            if (oty >= 3) u8g2_DrawStr(u8g2, 2, oty - 3, oldp->title);
        }

        /* 旧页选择条 — 直接跟随被选项文字位置 (过渡期间不限位) */
        u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
        if (oldp->count > 0) {
            uint8_t os = state->trans_old_sel;
            int16_t ascent2 = u8g2_GetAscent(u8g2);
            int16_t box_top = state->items_old[os].cur_y - ascent2 - BOX_PAD_Y;
            u8g2_uint_t sw = u8g2_GetStrWidth(u8g2, oldp->items[os].name);
            u8g2_SetDrawColor(u8g2, 2);
            u8g2_DrawRBox(u8g2, 2, box_top, (int16_t)sw + BOX_PAD_X * 2, MENU_LINE_HEIGHT, BOX_RADIUS);
        }

    } else if (state->trans == TRANS_NEW_IN) {
        /* ---- 新页进入 (全部从 y=-12 向下展开) ---- */
        const menu_page_t *newp = state->current;
        int16_t ttl = VISIBLE_TOP;
        int16_t box_h = MENU_LINE_HEIGHT;

        u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
        int16_t ascent = u8g2_GetAscent(u8g2);

        /* 新页项 */
        for (uint8_t i = 0; i < newp->count && i < MENU_MAX_ITEMS; i++) {
            int16_t y = state->items_new[i].cur_y;
            if (y < 2 || y > 65) continue;
            u8g2_SetDrawColor(u8g2, 1);
            u8g2_DrawStr(u8g2, 4 + BOX_PAD_X, y, newp->items[i].name);
        }

        /* 新页标题栏 (黑底白字) */
        {
            int16_t nty = state->title_new.cur_y;
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawBox(u8g2, 0, nty - ttl + 1, 128, ttl);
            u8g2_SetDrawColor(u8g2, 1);
            u8g2_DrawHLine(u8g2, 0, nty, 128);
            u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
            if (nty >= 3) u8g2_DrawStr(u8g2, 2, nty - 3, newp->title);
        }

        /* 选择条 — 直接跟随新页第一项文字位置 (过渡期间不限位) */
        u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
        if (newp->count > 0) {
            int16_t box_top = state->items_new[0].cur_y - ascent - BOX_PAD_Y;
            u8g2_uint_t sw = u8g2_GetStrWidth(u8g2, newp->items[0].name);
            u8g2_SetDrawColor(u8g2, 2);
            u8g2_DrawRBox(u8g2, 2, box_top, (int16_t)sw + BOX_PAD_X * 2, box_h, BOX_RADIUS);
        }

    } else {
        /* ---- normal render ---- */
        int16_t scroll = state->scroll_anim.cur_y;
        const menu_page_t *page = state->current;
        uint8_t sel = state->selected;

        u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
        int16_t ascent = u8g2_GetAscent(u8g2);
        int16_t box_h  = MENU_LINE_HEIGHT;

        /* bar target */
        int16_t item_y     = VISIBLE_TOP + (int16_t)sel * MENU_LINE_HEIGHT - scroll + ascent;
        int16_t targ_box_y = item_y - ascent - BOX_PAD_Y;
        u8g2_uint_t str_w = u8g2_GetStrWidth(u8g2, page->items[sel].name);
        int16_t targ_box_w = (int16_t)str_w + BOX_PAD_X * 2;

        /* 限制选择条最大宽度: 比右侧进度条(x=125)小1像素, 即右边界 ≤124 */
        if (targ_box_w > BAR_MAX_W) targ_box_w = BAR_MAX_W;

        if (targ_box_y < VISIBLE_TOP + 1)   targ_box_y = VISIBLE_TOP + 1;
        if (targ_box_y > 64 - box_h)        targ_box_y = 64 - box_h;

        if (targ_box_y != state->bar_target_y || targ_box_w != state->bar_target_w) {
            int16_t start_y = state->bar_anim.cur_y;
            int16_t start_w = state->bar_anim.cur_x;
            if (state->bar_target_y < 0) { start_y = targ_box_y; start_w = targ_box_w; }
            anim_start(&state->bar_anim, start_w, start_y,
                       targ_box_w, targ_box_y, BAR_ANIM_MS, quad_ease_out);
            state->bar_target_y = targ_box_y;
            state->bar_target_w = targ_box_w;
        }

        /* ---- 选中项文字水平滚动 (文字超出可见区域时, 线性插值向左滚动) ---- */
        {
            int16_t text_overflow = (int16_t)str_w - TEXT_VISIBLE_W;
            if (text_overflow > 0) {
                int16_t targ = text_overflow;
                if (targ != state->text_scroll_target) {
                    int16_t start_s = state->text_scroll_anim.cur_x;
                    if (state->text_scroll_target < 0) start_s = 0;
                    /* 动画时长与溢出量成比例: 每像素约12ms, 确保线性滚动速度均匀 */
                    uint32_t dur = (uint32_t)text_overflow * 12;
                    if (dur < SCROLL_ANIM_MS) dur = SCROLL_ANIM_MS;
                    anim_start(&state->text_scroll_anim, start_s, 0,
                               targ, 0, dur, linear_ease);
                    state->text_scroll_target = targ;
                }
            } else {
                /* 文字不溢出: 复位滚动偏移 */
                if (state->text_scroll_target != 0) {
                    int16_t start_s = state->text_scroll_anim.cur_x;
                    uint32_t dur = (uint32_t)(start_s > 0 ? start_s * 8 : SCROLL_ANIM_MS / 2);
                    anim_start(&state->text_scroll_anim, start_s, 0,
                               0, 0, dur, linear_ease);
                    state->text_scroll_target = 0;
                }
            }
        }
        int16_t text_scroll_offs = state->text_scroll_anim.cur_x;

        /* ---- pass 1: all items normal text (选中项应用水平滚动偏移) ---- */
        for (uint8_t i = 0; i < page->count; i++) {
            int16_t y = VISIBLE_TOP + (int16_t)i * MENU_LINE_HEIGHT - scroll + ascent;
            if (y < VISIBLE_TOP || y > 65) continue;
            u8g2_SetDrawColor(u8g2, 1);
            int16_t tx = TEXT_START_X;
            if (i == sel) tx -= text_scroll_offs;
            u8g2_DrawStr(u8g2, tx, y, page->items[i].name);
        }

        /* ---- pass 2: XOR box at animated position ---- */
        {
            int16_t bar_y = state->bar_anim.cur_y;
            int16_t bar_w = state->bar_anim.cur_x;
            int16_t bx    = 2;
            u8g2_SetDrawColor(u8g2, 2);
            u8g2_DrawRBox(u8g2, bx, bar_y, bar_w, box_h, BOX_RADIUS);
        }

        /* ---- scroll progress indicator (right edge, animated) ---- */
        if (page->count > 1) {
            int16_t max_h = 64 - VISIBLE_TOP;
            int16_t targ = (int16_t)state->selected * max_h / (int16_t)(page->count - 1);
            if (targ != state->prog_target) {
                int16_t start_h = state->prog_anim.cur_y;
                if (state->prog_target < 0) start_h = targ;
                anim_start(&state->prog_anim, 0, start_h, 0, targ,
                           BAR_ANIM_MS, quad_ease_out);
                state->prog_target = targ;
            }
            int16_t h = state->prog_anim.cur_y;
            if (h > 0) {
                /* 左侧 1px 黑色描边, 分隔索引条与菜单文字 */
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawBox(u8g2, 124, VISIBLE_TOP, 1, h);
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawBox(u8g2, 125, VISIBLE_TOP, 3, h);
            }
        }

        /* ---- title bar (black fill, white text + separator) ---- */
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, 0, 0, 128, VISIBLE_TOP);
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
        u8g2_DrawStr(u8g2, 2, VISIBLE_TOP - 3, page->title);
        u8g2_DrawHLine(u8g2, 0, VISIBLE_TOP - 1, 128);
    }
}

/* ======== key handlers ======== */

bool menu_key_up(menu_state_t *state) {
    if (state->trans != TRANS_NONE) return false;
    if (state->selected > 0) {
        state->selected--;
        start_scroll(state);
        return true;
    }
    return false;
}

bool menu_key_down(menu_state_t *state) {
    if (state->trans != TRANS_NONE) return false;
    if (state->selected + 1 < state->current->count) {
        state->selected++;
        start_scroll(state);
        return true;
    }
    return false;
}

/*
 *  阶段 1: 旧页退出 — 全部向 y = -12 向上收缩
 *  阶段 2: 新页进入 — 全部从 y = -12 向下展开 (由 menu_update 触发)
 *  每项一个 anim_ctrl_t, 时长统一 TRANS_MS / 2, 共用 quad_ease_out
 */
static void trans_start_old(menu_state_t *state, int16_t ascent) {
    const menu_page_t *oldp = state->trans_old;
    int16_t ttl = VISIBLE_TOP;
    int16_t end = -12;
    uint16_t dur = TRANS_MS / 2;

    anim_start(&state->title_old, 0, ttl - 1, 0, end, dur, quad_ease_out);
    for (uint8_t i = 0; i < oldp->count && i < MENU_MAX_ITEMS; i++) {
        int16_t y = ttl + (int16_t)i * MENU_LINE_HEIGHT + ascent;
        anim_start(&state->items_old[i], 0, y, 0, end, dur, quad_ease_out);
    }

    /* 选择条 — 跟随旧页选中项一起移动, 位置在 render 中实时计算 */
    state->bar_target_y = -1;            /* 宽度由 render 首次测量 */
    state->bar_target_w = -1;
}

static void trans_start_new(menu_state_t *state, int16_t ascent) {
    const menu_page_t *newp = state->current;
    int16_t ttl = VISIBLE_TOP;
    int16_t end = -12;
    uint16_t dur = TRANS_MS / 2;

    anim_start(&state->title_new, 0, end, 0, ttl - 1, dur, quad_ease_out);
    for (uint8_t i = 0; i < newp->count && i < MENU_MAX_ITEMS; i++) {
        int16_t y = ttl + (int16_t)i * MENU_LINE_HEIGHT + ascent;
        anim_start(&state->items_new[i], 0, end, 0, y, dur, quad_ease_out);
    }

    /* 选择条 — 跟随新页第一项一起移动, 位置在 render 中实时计算 */
    state->bar_target_y = -1;
    state->bar_target_w = -1;
}

bool menu_key_enter(menu_state_t *state) {
    if (state->trans != TRANS_NONE) return false;
    const menu_item_t *item = &state->current->items[state->selected];
    if (item->action) {
        item->action();
        return true;
    }
    if (item->submenu) {
        state->trans_old     = state->current;
        state->trans_old_sel = state->selected;
        state->current       = item->submenu;
        state->selected      = 0;
        state->trans         = TRANS_OLD_OUT;
        trans_start_old(state, 8);        /* helvB10 ascent ≈ 8 */
        return true;
    }
    return false;
}

bool menu_key_back(menu_state_t *state) {
    if (state->trans != TRANS_NONE) return false;
    if (state->current->parent) {
        state->trans_old     = state->current;
        state->trans_old_sel = state->selected;
        state->current       = state->current->parent;
        state->selected      = 0;
        state->trans         = TRANS_OLD_OUT;
        trans_start_old(state, 8);
        return true;
    }
    return false;
}
