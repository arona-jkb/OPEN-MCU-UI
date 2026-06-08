/**
 * 菜单模块实现
 *
 *  文字菜单: 垂直排列, 选择条动画, 边界触发滚动
 *  图标菜单: 选择框固定屏幕中央, 选中项居中, 其余向两侧展开
 */
#include "menu.h"
#include "ui_timing.h"

/* ---- 文字菜单 ---- */
#define BOX_PAD_X    5
#define BOX_PAD_Y    1
#define BOX_RADIUS   1
#define BAR_MAX_W      122
#define TEXT_START_X   (4 + BOX_PAD_X)
#define TEXT_MAX_END   (2 + BAR_MAX_W - BOX_PAD_X)
#define TEXT_VISIBLE_W (TEXT_MAX_END - TEXT_START_X)
#define TEXT_TOP_PAD 3

/* ---- 图标菜单 ---- */
#define ICON_GAP          8
#define ICON_FRAME_GAP    4
#define ICON_BRACKET_LEN  6
#define ICON_BRACKET_THICK 2
#define FRAME_CENTER_X    64              /* 选择框水平固定中心 */

#define DRAW_ICON_BRACKETS(u8g2, fx, fy, fw, fh) do {                   \
    int16_t _bx = (int16_t)(fx), _by = (int16_t)(fy);                   \
    int16_t _bw = (int16_t)(fw), _bh = (int16_t)(fh);                   \
    int16_t _bl = ICON_BRACKET_LEN, _bt = ICON_BRACKET_THICK;           \
    u8g2_DrawBox(u8g2, _bx,              _by,              _bl, _bt);   \
    u8g2_DrawBox(u8g2, _bx,              _by,              _bt, _bl);   \
    u8g2_DrawBox(u8g2, _bx+_bw-_bl,      _by,              _bl, _bt);   \
    u8g2_DrawBox(u8g2, _bx+_bw-_bt,      _by,              _bt, _bl);   \
    u8g2_DrawBox(u8g2, _bx,              _by+_bh-_bt,      _bl, _bt);   \
    u8g2_DrawBox(u8g2, _bx,              _by+_bh-_bl,      _bt, _bl);   \
    u8g2_DrawBox(u8g2, _bx+_bw-_bl,      _by+_bh-_bt,      _bl, _bt);   \
    u8g2_DrawBox(u8g2, _bx+_bw-_bt,      _by+_bh-_bl,      _bt, _bl);   \
} while(0)

/* ---- 过渡时长 ---- */
#define ICON_TRANS_TITLE_MS 400
#define ICON_TRANS_LABEL_MS 400
#define ICON_TRANS_PROG_MS  400
#define ICON_TRANS_ICON_BASE 200
#define ICON_TRANS_ICON_STEP 80

/* ---- 可见区 ---- */
#define VISIBLE_TOP    MENU_TITLE_HEIGHT
#define VISIBLE_BOTTOM (64 - MENU_LINE_HEIGHT)

/* ======== 前向声明 ======== */
static int16_t calc_scroll_target(const menu_state_t *state);
static void start_scroll(menu_state_t *state);
static void trans_start_old(menu_state_t *state, int16_t ascent);
static void trans_start_new_text(menu_state_t *state, int16_t ascent);
static void icon_trans_start(menu_state_t *state);
static void icon_trans_start_exit(menu_state_t *state);
static void icon_label_swap(menu_state_t *state, uint8_t old_sel, uint8_t new_sel);

/* ======== 文字菜单滚动 ======== */
static int16_t calc_scroll_target(const menu_state_t *state) {
    uint8_t n = state->current->count;
    if (n == 0) return 0;
    int16_t cur = state->scroll_anim.cur_y;
    int16_t list_top = VISIBLE_TOP + TEXT_TOP_PAD;
    int16_t item_y = list_top + (int16_t)state->selected * MENU_LINE_HEIGHT - cur;
    if (item_y < list_top) {
        int16_t t = (int16_t)state->selected * MENU_LINE_HEIGHT;
        return t < 0 ? 0 : t;
    }
    if (item_y > VISIBLE_BOTTOM) {
        int16_t max_scroll = list_top + (int16_t)(n - 1) * MENU_LINE_HEIGHT - VISIBLE_BOTTOM;
        if (max_scroll < 0) max_scroll = 0;
        int16_t t = list_top + (int16_t)state->selected * MENU_LINE_HEIGHT - VISIBLE_BOTTOM;
        if (t > max_scroll) t = max_scroll;
        return t < 0 ? 0 : t;
    }
    return cur;
}

static void start_scroll(menu_state_t *state) {
    int16_t target = calc_scroll_target(state);
    state->scroll_target = target;
    anim_start(&state->scroll_anim, 0, state->scroll_anim.cur_y, 0, target,
               SCROLL_ANIM_MS, quad_ease_out);
    state->text_scroll_target = -1;
    anim_stop(&state->text_scroll_anim);
    anim_set_position(&state->text_scroll_anim, 0, 0);
}

/* ======== 图标菜单: 选中项变化时滚动动画 ======== */
static void icon_sel_scroll(menu_state_t *state, int16_t slot_step) {
    int16_t targ = (int16_t)state->selected * slot_step;
    if (targ != state->icon_scroll_target) {
        int16_t cur = state->icon_scroll_anim.cur_x;
        if (state->icon_scroll_target < 0) cur = targ;
        anim_start(&state->icon_scroll_anim, cur, 0, targ, 0,
                   SCROLL_ANIM_MS, quad_ease_out);
        state->icon_scroll_target = targ;
    }
}

/* ======== 初始化 ======== */
void menu_init(menu_state_t *state, const menu_page_t *root) {
    state->current = root;
    state->selected = 0;
    state->scroll_target = 0;
    anim_init(&state->scroll_anim);
    anim_set_position(&state->scroll_anim, 0, 0);
    anim_init(&state->text_scroll_anim);
    anim_set_position(&state->text_scroll_anim, 0, 0);
    state->text_scroll_target = -1;
    anim_init(&state->icon_scroll_anim);
    anim_set_position(&state->icon_scroll_anim, 0, 0);
    state->icon_scroll_target = -1;
    anim_init(&state->icon_trans_title_y);
    anim_set_position(&state->icon_trans_title_y, 0, 0);
    anim_init(&state->icon_trans_label_y);
    anim_set_position(&state->icon_trans_label_y, 0, 0);
    for (int i = 0; i < MENU_MAX_ITEMS; i++) {
        anim_init(&state->icon_trans_item_x[i]);
        anim_set_position(&state->icon_trans_item_x[i], 0, 0);
    }
    anim_init(&state->icon_label_old_y);
    anim_set_position(&state->icon_label_old_y, 0, 60);
    anim_init(&state->icon_label_new_y);
    anim_set_position(&state->icon_label_new_y, 0, 64);
    state->icon_label_old_name = NULL;
    state->icon_label_new_name = NULL;
    state->icon_label_phase    = 0;
    anim_init(&state->bar_anim);
    state->bar_target_y = -1;
    state->bar_target_w = -1;
    anim_init(&state->prog_anim);
    state->prog_target = -1;
    state->cached_slot_step = -1;
    state->cached_count     = 0;
    state->cached_iw        = 0;
    state->trans = TRANS_NONE;
    anim_init(&state->title_old);
    anim_init(&state->title_new);
    for (int i = 0; i < MENU_MAX_ITEMS; i++) {
        anim_init(&state->items_old[i]);
        anim_init(&state->items_new[i]);
    }
}

/* ======== 状态更新 ======== */
void menu_update(menu_state_t *state) {
    if (state->trans == TRANS_NONE) return;

    if (state->trans == TRANS_OLD_OUT) {
        if (state->trans_old->style == MENU_ICON) {
            uint8_t n = state->trans_old->count;
            if (n > 0) {
                uint8_t last = n - 1;
                if (last >= MENU_MAX_ITEMS) last = MENU_MAX_ITEMS - 1;
                if (state->icon_trans_item_x[last].state == ANIM_FINISHED ||
                    state->icon_trans_item_x[last].state == ANIM_IDLE) goto done;
            } else {
                if (state->icon_trans_title_y.state == ANIM_FINISHED ||
                    state->icon_trans_title_y.state == ANIM_IDLE) goto done;
            }
        } else {
            if (state->title_old.state == ANIM_FINISHED ||
                state->title_old.state == ANIM_IDLE) goto done;
        }
        return;
    done:
        state->trans = TRANS_NEW_IN;
        if (state->current->style == MENU_ICON)
            icon_trans_start(state);
        else
            trans_start_new_text(state, 7);
    } else if (state->trans == TRANS_NEW_IN) {
        if (state->current->style == MENU_ICON) {
            uint8_t n = state->current->count;
            if (n > 0) {
                uint8_t last = n - 1;
                if (last >= MENU_MAX_ITEMS) last = MENU_MAX_ITEMS - 1;
                if (state->icon_trans_item_x[last].state == ANIM_FINISHED ||
                    state->icon_trans_item_x[last].state == ANIM_IDLE) {
                    state->trans = TRANS_NONE;
                    state->icon_scroll_target = -1;
                    state->prog_target  = -1;
                    state->icon_label_old_name = NULL;
                    state->icon_label_new_name = NULL;
                    state->icon_label_phase    = 0;
                }
            } else {
                if (state->icon_trans_title_y.state == ANIM_FINISHED ||
                    state->icon_trans_title_y.state == ANIM_IDLE) {
                    state->trans = TRANS_NONE;
                    state->icon_scroll_target = -1;
                    state->prog_target  = -1;
                }
            }
        } else {
            if (state->title_new.state == ANIM_FINISHED ||
                state->title_new.state == ANIM_IDLE) {
                state->trans = TRANS_NONE;
                anim_set_position(&state->scroll_anim, 0, 0);
                state->scroll_target = 0;
                state->bar_target_y = -1;
                state->bar_target_w = -1;
                state->text_scroll_target = -1;
                state->icon_scroll_target = -1;
                state->prog_target  = -1;
            }
        }
    }
}

/* ======== 图标菜单布局计算 ======== */
/* 根据 count 返回 slot_step = slot_w + ICON_GAP */
/* 返回 slot_w + ICON_GAP, 结果缓存在 state->cached_slot_step */
static int16_t icon_layout(menu_state_t *state, uint8_t n, uint8_t iw) {
    if (state->cached_count == n && state->cached_iw == iw && state->cached_slot_step >= 0)
        return state->cached_slot_step;
    int16_t total_gap = (int16_t)(n - 1) * ICON_GAP;
    int16_t slot_w = (TEXT_VISIBLE_W - total_gap) / (int16_t)n;
    if (slot_w < (int16_t)iw + ICON_FRAME_GAP * 2)
        slot_w = (int16_t)iw + ICON_FRAME_GAP * 2;
    int16_t step = slot_w + ICON_GAP;
    state->cached_slot_step = step;
    state->cached_count     = n;
    state->cached_iw        = iw;
    return step;
}

/* 选中项绝对中心 X (滚动前) */
#define ICON_SEL_ABS_CX   FRAME_CENTER_X

/* 第 i 项绝对中心 X */
static int16_t icon_abs_cx(uint8_t i, uint8_t sel, int16_t step) {
    return FRAME_CENTER_X + ((int16_t)i - (int16_t)sel) * step;
}

/* 绘制标题栏文字 (根据 page->title_align 决定对齐) */
static void draw_page_title(u8g2_t *u8g2, const menu_page_t *page, int16_t y) {
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    if (page->title_align == TITLE_CENTER) {
        u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, page->title);
        int16_t tx = (int16_t)(128 - tw) / 2; if (tx < 0) tx = 0;
        u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, y, page->title);
    } else {
        u8g2_DrawStr(u8g2, 2, y, page->title);
    }
}

/* ======== main render ======== */
void menu_render(u8g2_t *u8g2, menu_state_t *state) {
    u8g2_SetFontMode(u8g2, 1);

    /* ============================================================
     *  TRANS_OLD_OUT
     * ============================================================ */
    if (state->trans == TRANS_OLD_OUT) {
        const menu_page_t *oldp = state->trans_old;

        if (oldp->style == MENU_ICON) {
            uint8_t n = oldp->count, os = state->trans_old_sel;
            const menu_item_t *sel_item = &oldp->items[os];
            uint8_t iw = sel_item->icon.w, ih = sel_item->icon.h;
            int16_t frame_pad = ICON_FRAME_GAP;
            int16_t icon_y = VISIBLE_TOP + 8;

            /* 标题栏 */
            {
                int16_t ttl_y = state->icon_trans_title_y.cur_y;
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawBox(u8g2, 0, ttl_y - VISIBLE_TOP + 1, 128, VISIBLE_TOP);
                u8g2_SetDrawColor(u8g2, 1);
                draw_page_title(u8g2, oldp, ttl_y - 3);
                if (n > 1) { int16_t pw = state->prog_anim.cur_x;
                    if (pw > 0) u8g2_DrawBox(u8g2, 0, VISIBLE_TOP, (u8g2_uint_t)pw, 3); }
            }

            /* 选中框 (固定中央) */
            {
                int16_t fx = FRAME_CENTER_X - (int16_t)iw / 2 - frame_pad;
                int16_t fy = icon_y - frame_pad;
                u8g2_SetDrawColor(u8g2, 2);
                DRAW_ICON_BRACKETS(u8g2, fx, fy, (int16_t)iw + frame_pad * 2,
                                   (int16_t)ih + frame_pad * 2);
            }

            /* 图标: 末→首 (首项在最上层), 各自飞向中心 */
            u8g2_SetDrawColor(u8g2, 1);
            for (int8_t i = (int8_t)(n - 1); i >= 0; i--) {
                if (i >= MENU_MAX_ITEMS) continue;
                int16_t ix = state->icon_trans_item_x[i].cur_x;
                u8g2_DrawXBMP(u8g2, (u8g2_uint_t)ix, (u8g2_uint_t)icon_y,
                              oldp->items[i].icon.w, oldp->items[i].icon.h,
                              oldp->items[i].icon.bitmap);
            }

            /* 标签 */
            {
                int16_t label_y = state->icon_trans_label_y.cur_y;
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
                u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, sel_item->name);
                int16_t tx = (int16_t)(128 - tw) / 2; if (tx < 0) tx = 0;
                u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, (u8g2_uint_t)label_y, sel_item->name);
                if (n > 1) { u8g2_DrawStr(u8g2, 3, (u8g2_uint_t)label_y, "<");
                             u8g2_DrawStr(u8g2, 119, (u8g2_uint_t)label_y, ">"); }
            }

        } else {
            /* 文字菜单退出 */
            int16_t ttl = VISIBLE_TOP;
            u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
            for (uint8_t i = 0; i < oldp->count && i < MENU_MAX_ITEMS; i++) {
                int16_t y = state->items_old[i].cur_y;
                if (y < -10 || y > 65) continue;
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawStr(u8g2, 4 + BOX_PAD_X, y, oldp->items[i].name);
            }
            {   int16_t oty = state->title_old.cur_y;
                u8g2_SetDrawColor(u8g2, 0); u8g2_DrawBox(u8g2, 0, oty - ttl + 1, 128, ttl);
                u8g2_SetDrawColor(u8g2, 1); u8g2_DrawHLine(u8g2, 0, oty, 128);
                if (oty >= 3) draw_page_title(u8g2, oldp, oty - 3); }
            u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
            if (oldp->count > 0) {
                uint8_t os = state->trans_old_sel;
                int16_t a = u8g2_GetAscent(u8g2);
                int16_t bt = state->items_old[os].cur_y - a - BOX_PAD_Y;
                u8g2_uint_t sw = u8g2_GetStrWidth(u8g2, oldp->items[os].name);
                u8g2_SetDrawColor(u8g2, 2);
                u8g2_DrawRBox(u8g2, 2, bt, (int16_t)sw + BOX_PAD_X * 2, MENU_LINE_HEIGHT, BOX_RADIUS);
            }
        }

    /* ============================================================
     *  TRANS_NEW_IN
     * ============================================================ */
    } else if (state->trans == TRANS_NEW_IN) {
        const menu_page_t *newp = state->current;

        if (newp->style == MENU_ICON) {
            uint8_t n = newp->count;
            const menu_item_t *sel_item = &newp->items[0];
            uint8_t iw = sel_item->icon.w, ih = sel_item->icon.h;
            int16_t frame_pad = ICON_FRAME_GAP;
            int16_t icon_y = VISIBLE_TOP + 8;

            /* 标题栏 */
            {
                int16_t ttl_y = state->icon_trans_title_y.cur_y;
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawBox(u8g2, 0, ttl_y - VISIBLE_TOP + 1, 128, VISIBLE_TOP);
                u8g2_SetDrawColor(u8g2, 1);
                draw_page_title(u8g2, newp, ttl_y - 3);
                if (n > 1) { int16_t pw = state->prog_anim.cur_x;
                    if (pw > 0) u8g2_DrawBox(u8g2, 0, VISIBLE_TOP, (u8g2_uint_t)pw, 3); }
            }

            /* 选中框 (固定中央) */
            {
                int16_t fx = FRAME_CENTER_X - (int16_t)iw / 2 - frame_pad;
                int16_t fy = icon_y - frame_pad;
                u8g2_SetDrawColor(u8g2, 2);
                DRAW_ICON_BRACKETS(u8g2, fx, fy, (int16_t)iw + frame_pad * 2,
                                   (int16_t)ih + frame_pad * 2);
            }

            /* 图标: 末→首, 从中心飞向目标位 */
            u8g2_SetDrawColor(u8g2, 1);
            for (int8_t i = (int8_t)(n - 1); i >= 0; i--) {
                if (i >= MENU_MAX_ITEMS) continue;
                int16_t ix = state->icon_trans_item_x[i].cur_x;
                u8g2_DrawXBMP(u8g2, (u8g2_uint_t)ix, (u8g2_uint_t)icon_y,
                              newp->items[i].icon.w, newp->items[i].icon.h,
                              newp->items[i].icon.bitmap);
            }

            /* 标签 */
            {
                int16_t label_y = state->icon_trans_label_y.cur_y;
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
                u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, sel_item->name);
                int16_t tx = (int16_t)(128 - tw) / 2; if (tx < 0) tx = 0;
                u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, (u8g2_uint_t)label_y, sel_item->name);
                if (n > 1) { u8g2_DrawStr(u8g2, 3, (u8g2_uint_t)label_y, "<");
                             u8g2_DrawStr(u8g2, 119, (u8g2_uint_t)label_y, ">"); }
            }

        } else {
            /* 文字菜单入场 */
            int16_t ttl = VISIBLE_TOP, box_h = MENU_LINE_HEIGHT;
            u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
            int16_t ascent = u8g2_GetAscent(u8g2);
            for (uint8_t i = 0; i < newp->count && i < MENU_MAX_ITEMS; i++) {
                int16_t y = state->items_new[i].cur_y;
                if (y < 2 || y > 65) continue;
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawStr(u8g2, 4 + BOX_PAD_X, y, newp->items[i].name);
            }
            {   int16_t nty = state->title_new.cur_y;
                u8g2_SetDrawColor(u8g2, 0); u8g2_DrawBox(u8g2, 0, nty - ttl + 1, 128, ttl);
                u8g2_SetDrawColor(u8g2, 1); u8g2_DrawHLine(u8g2, 0, nty, 128);
                if (nty >= 3) draw_page_title(u8g2, newp, nty - 3); }
            u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
            if (newp->count > 0) {
                int16_t bt = state->items_new[0].cur_y - ascent - BOX_PAD_Y;
                u8g2_uint_t sw = u8g2_GetStrWidth(u8g2, newp->items[0].name);
                u8g2_SetDrawColor(u8g2, 2);
                u8g2_DrawRBox(u8g2, 2, bt, (int16_t)sw + BOX_PAD_X * 2, box_h, BOX_RADIUS);
            }
        }

    /* ============================================================
     *  TRANS_NONE — 正常渲染
     * ============================================================ */
    } else {
        int16_t scroll = state->scroll_anim.cur_y;
        const menu_page_t *page = state->current;
        uint8_t sel = state->selected;

        /* ============================================================
         *  图标菜单: 框固定中央, 选中项居中, 其余向两侧展开
         * ============================================================ */
        if (page->style == MENU_ICON) {
            uint8_t n = page->count;
            if (n == 0) goto draw_title;

            const menu_item_t *sel_item = &page->items[sel];
            uint8_t iw = sel_item->icon.w, ih = sel_item->icon.h;
            int16_t frame_pad = ICON_FRAME_GAP;
            int16_t icon_y = VISIBLE_TOP + 8;
            int16_t step = icon_layout(state, n, iw);

            /* 滚动目标: 选中项居中 = sel * step */
            icon_sel_scroll(state, step);
            int16_t scroll_offs = state->icon_scroll_anim.cur_x;

            /* pass 1: 所有图标 */
            u8g2_SetDrawColor(u8g2, 1);
            for (uint8_t i = 0; i < n; i++) {
                int16_t abs_cx = icon_abs_cx(i, 0, step);  /* 以 item0 为原点 */
                if (abs_cx - (int16_t)page->items[i].icon.w / 2 - scroll_offs + (int16_t)page->items[i].icon.w < TEXT_START_X ||
                    abs_cx - (int16_t)page->items[i].icon.w / 2 - scroll_offs > TEXT_MAX_END) continue;
                int16_t ix = abs_cx - scroll_offs - (int16_t)page->items[i].icon.w / 2;
                u8g2_DrawXBMP(u8g2, (u8g2_uint_t)ix, (u8g2_uint_t)icon_y,
                              page->items[i].icon.w, page->items[i].icon.h,
                              page->items[i].icon.bitmap);
            }

            /* pass 2: 选中框 (固定中央) */
            {
                int16_t fx = FRAME_CENTER_X - (int16_t)iw / 2 - frame_pad;
                int16_t fy = icon_y - frame_pad;
                u8g2_SetDrawColor(u8g2, 2);
                DRAW_ICON_BRACKETS(u8g2, fx, fy, (int16_t)iw + frame_pad * 2,
                                   (int16_t)ih + frame_pad * 2);
            }

            /* 标签 */
            {
                u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
                u8g2_SetDrawColor(u8g2, 1);
                if (state->icon_label_phase == 1) {
                    int16_t old_y = state->icon_label_old_y.cur_y;
                    u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, state->icon_label_old_name);
                    int16_t tx = (int16_t)(128 - tw) / 2; if (tx < 0) tx = 0;
                    u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, (u8g2_uint_t)old_y, state->icon_label_old_name);
                    if (state->icon_label_old_y.state == ANIM_FINISHED || state->icon_label_old_y.state == ANIM_IDLE) {
                        anim_start(&state->icon_label_new_y, 0, 64, 0, 60, BAR_ANIM_MS, quad_ease_out);
                        state->icon_label_phase = 2;
                    }
                } else if (state->icon_label_phase == 2) {
                    int16_t new_y = state->icon_label_new_y.cur_y;
                    u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, state->icon_label_new_name);
                    int16_t tx = (int16_t)(128 - tw) / 2; if (tx < 0) tx = 0;
                    u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, (u8g2_uint_t)new_y, state->icon_label_new_name);
                    if (state->icon_label_new_y.state == ANIM_FINISHED || state->icon_label_new_y.state == ANIM_IDLE) {
                        state->icon_label_phase = 0; state->icon_label_old_name = NULL; state->icon_label_new_name = NULL;
                    }
                } else {
                    u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, sel_item->name);
                    int16_t tx = (int16_t)(128 - tw) / 2; if (tx < 0) tx = 0;
                    u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, 60, sel_item->name);
                }
            }

            /* 左右箭头提示 (n>1 时绘制) */
            if (n > 1) {
                u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawStr(u8g2, 3, 60, "<");
                u8g2_DrawStr(u8g2, 119, 60, ">");
            }

            /* 进度条 */
            if (n > 1) {
                int16_t targ = (int16_t)(sel + 1) * 124 / (int16_t)n;
                if (targ != state->prog_target) {
                    int16_t sw = state->prog_anim.cur_x;
                    if (state->prog_target < 0) sw = targ;
                    anim_start(&state->prog_anim, sw, 0, targ, 0, BAR_ANIM_MS, quad_ease_out);
                    state->prog_target = targ;
                }
            }

            goto draw_title;
        }

        /* ============================================================
         *  文字菜单
         * ============================================================ */
        u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
        int16_t ascent = u8g2_GetAscent(u8g2);
        int16_t box_h  = MENU_LINE_HEIGHT;

        int16_t item_y     = VISIBLE_TOP + TEXT_TOP_PAD + (int16_t)sel * MENU_LINE_HEIGHT - scroll + ascent;
        int16_t targ_box_y = item_y - ascent - BOX_PAD_Y;
        u8g2_uint_t str_w = u8g2_GetStrWidth(u8g2, page->items[sel].name);
        int16_t targ_box_w = (int16_t)str_w + BOX_PAD_X * 2;
        if (targ_box_w > BAR_MAX_W) targ_box_w = BAR_MAX_W;
        if (targ_box_y < VISIBLE_TOP)        targ_box_y = VISIBLE_TOP;
        if (targ_box_y > 64 - box_h)         targ_box_y = 64 - box_h;

        if (targ_box_y != state->bar_target_y || targ_box_w != state->bar_target_w) {
            int16_t sy = state->bar_anim.cur_y, sw = state->bar_anim.cur_x;
            if (state->bar_target_y < 0) { sy = targ_box_y; sw = targ_box_w; }
            anim_start(&state->bar_anim, sw, sy, targ_box_w, targ_box_y, BAR_ANIM_MS, quad_ease_out);
            state->bar_target_y = targ_box_y; state->bar_target_w = targ_box_w;
        }

        /* 文字水平滚动 */
        {
            int16_t ovf = (int16_t)str_w - TEXT_VISIBLE_W;
            if (ovf > 0) {
                if (ovf != state->text_scroll_target) {
                    int16_t ss = state->text_scroll_anim.cur_x;
                    if (state->text_scroll_target < 0) ss = 0;
                    uint32_t dur = (uint32_t)ovf * 12; if (dur < SCROLL_ANIM_MS) dur = SCROLL_ANIM_MS;
                    anim_start(&state->text_scroll_anim, ss, 0, ovf, 0, dur, linear_ease);
                    state->text_scroll_target = ovf;
                }
            } else {
                if (state->text_scroll_target != 0) {
                    int16_t ss = state->text_scroll_anim.cur_x;
                    uint32_t dur = (uint32_t)(ss > 0 ? ss * 8 : SCROLL_ANIM_MS / 2);
                    anim_start(&state->text_scroll_anim, ss, 0, 0, 0, dur, linear_ease);
                    state->text_scroll_target = 0;
                }
            }
        }
        int16_t ts = state->text_scroll_anim.cur_x;

        /* pass 1 */
        for (uint8_t i = 0; i < page->count; i++) {
            int16_t y = VISIBLE_TOP + TEXT_TOP_PAD + (int16_t)i * MENU_LINE_HEIGHT - scroll + ascent;
            if (y < VISIBLE_TOP || y > 65) continue;
            u8g2_SetDrawColor(u8g2, 1);
            int16_t tx = TEXT_START_X; if (i == sel) tx -= ts;
            u8g2_DrawStr(u8g2, tx, y, page->items[i].name);
        }
        /* pass 2 */
        {
            int16_t by = state->bar_anim.cur_y, bw = state->bar_anim.cur_x;
            u8g2_SetDrawColor(u8g2, 2);
            u8g2_DrawRBox(u8g2, 2, by, bw, box_h, BOX_RADIUS);
        }
        /* 右侧进度条 */
        if (page->count > 1) {
            int16_t max_h = 64 - VISIBLE_TOP;
            int16_t targ = (int16_t)state->selected * max_h / (int16_t)(page->count - 1);
            if (targ != state->prog_target) {
                int16_t sh = state->prog_anim.cur_y;
                if (state->prog_target < 0) sh = targ;
                anim_start(&state->prog_anim, 0, sh, 0, targ, BAR_ANIM_MS, quad_ease_out);
                state->prog_target = targ;
            }
            int16_t h = state->prog_anim.cur_y;
            if (h > 0) {
                u8g2_SetDrawColor(u8g2, 0); u8g2_DrawBox(u8g2, 124, VISIBLE_TOP, 1, h);
                u8g2_SetDrawColor(u8g2, 1); u8g2_DrawBox(u8g2, 125, VISIBLE_TOP, 3, h);
            }
        }

    draw_title:
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, 0, 0, 128, VISIBLE_TOP);
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
        draw_page_title(u8g2, page, VISIBLE_TOP - 3);
        if (page->style == MENU_ICON && page->count > 1) {
            int16_t pw = state->prog_anim.cur_x;
            if (pw > 0) u8g2_DrawBox(u8g2, 0, VISIBLE_TOP, (u8g2_uint_t)pw, 3);
        } else if (page->style != MENU_ICON) {
            u8g2_DrawHLine(u8g2, 0, VISIBLE_TOP - 1, 128);
        }
    }
}

/* ======== key handlers ======== */

static void icon_label_swap(menu_state_t *state, uint8_t old_sel, uint8_t new_sel) {
    const menu_page_t *page = state->current;
    if (old_sel == new_sel) return;
    state->icon_label_old_name = page->items[old_sel].name;
    state->icon_label_new_name = page->items[new_sel].name;
    state->icon_label_phase    = 1;
    anim_start(&state->icon_label_old_y, 0, 60, 0, 64, BAR_ANIM_MS, quad_ease_out);
}

bool menu_key_up(menu_state_t *state) {
    if (state->trans != TRANS_NONE) return false;
    if (state->current->style == MENU_ICON) {
        if (state->selected > 0) {
            uint8_t old = state->selected;
            state->selected--;
            icon_label_swap(state, old, state->selected);
            return true;
        }
        return false;
    }
    if (state->selected > 0) { state->selected--; start_scroll(state); return true; }
    return false;
}

bool menu_key_down(menu_state_t *state) {
    if (state->trans != TRANS_NONE) return false;
    if (state->current->style == MENU_ICON) {
        uint8_t n = state->current->count;
        if (n > 0 && state->selected + 1 < n) {
            uint8_t old = state->selected;
            state->selected++;
            icon_label_swap(state, old, state->selected);
            return true;
        }
        return false;
    }
    if (state->selected + 1 < state->current->count) { state->selected++; start_scroll(state); return true; }
    return false;
}

/* ======== 页面切换 ======== */

static void trans_start_old(menu_state_t *state, int16_t ascent) {
    const menu_page_t *oldp = state->trans_old;
    if (oldp->style == MENU_ICON) { icon_trans_start_exit(state); return; }

    int16_t title_target = VISIBLE_TOP - 1;
    int16_t item_base    = VISIBLE_TOP + TEXT_TOP_PAD;
    int16_t end = -12;
    uint16_t dur = TRANS_MS / 2;
    anim_start(&state->title_old, 0, title_target, 0, end, dur, quad_ease_out);
    for (uint8_t i = 0; i < oldp->count && i < MENU_MAX_ITEMS; i++) {
        int16_t y = item_base + (int16_t)i * MENU_LINE_HEIGHT + ascent;
        anim_start(&state->items_old[i], 0, y, 0, end, dur, quad_ease_out);
    }
    state->bar_target_y = -1; state->bar_target_w = -1;
}

static void trans_start_new_text(menu_state_t *state, int16_t ascent) {
    const menu_page_t *newp = state->current;
    int16_t title_target = VISIBLE_TOP - 1;
    int16_t item_base    = VISIBLE_TOP + TEXT_TOP_PAD;
    int16_t end = -12;
    uint16_t dur = TRANS_MS / 2;
    anim_start(&state->title_new, 0, end, 0, title_target, dur, quad_ease_out);
    for (uint8_t i = 0; i < newp->count && i < MENU_MAX_ITEMS; i++) {
        int16_t y = item_base + (int16_t)i * MENU_LINE_HEIGHT + ascent;
        anim_start(&state->items_new[i], 0, end, 0, y, dur, quad_ease_out);
    }
    state->bar_target_y = -1; state->bar_target_w = -1;
}

/* 图标菜单退场: 所有图标飞向中央 */
static void icon_trans_start_exit(menu_state_t *state) {
    const menu_page_t *oldp = state->trans_old;
    uint8_t n = oldp->count;
    const menu_item_t *sel_item = &oldp->items[oldp->count > 0 ? state->trans_old_sel : 0];
    uint8_t iw = sel_item->icon.w;
    int16_t step = icon_layout(state, n, iw);

    /* 标题滑出 */
    anim_start(&state->icon_trans_title_y, 0, VISIBLE_TOP - 1, 0, -12,
               ICON_TRANS_TITLE_MS, quad_ease_out);
    /* 标签下沉 */
    anim_start(&state->icon_trans_label_y, 0, 60, 0, 64,
               ICON_TRANS_LABEL_MS, quad_ease_out);
    /* 进度条收缩 */
    {   int16_t sw = state->prog_anim.cur_x;
        anim_start(&state->prog_anim, sw, 0, 0, 0, ICON_TRANS_PROG_MS, quad_ease_out);
        state->prog_target = 0; }

    if (n == 0) return;

    /* 图标依次飞向中央 (末项先飞) */
    int16_t origin_x = FRAME_CENTER_X - (int16_t)iw / 2;
    for (uint8_t i = 0; i < n && i < MENU_MAX_ITEMS; i++) {
        int16_t start_x = (int16_t)(FRAME_CENTER_X + ((int16_t)i - (int16_t)state->trans_old_sel) * step)
                          - (int16_t)oldp->items[i].icon.w / 2;
        uint8_t rev_i = (uint8_t)(n - 1 - i);
        uint32_t dur = (uint32_t)ICON_TRANS_ICON_BASE + (uint32_t)rev_i * ICON_TRANS_ICON_STEP;
        anim_start(&state->icon_trans_item_x[i], start_x, 0, origin_x, 0, dur, quad_ease_out);
    }
}

/* 图标菜单入场: 图标从中央飞向目标位 */
static void icon_trans_start(menu_state_t *state) {
    const menu_page_t *newp = state->current;
    uint8_t n = newp->count;
    if (n == 0) return;

    const menu_item_t *sel_item = &newp->items[0];  /* sel=0 */
    uint8_t iw = sel_item->icon.w;
    int16_t step = icon_layout(state, n, iw);

    /* 标题滑入 */
    anim_start(&state->icon_trans_title_y, 0, -12, 0, VISIBLE_TOP - 1,
               ICON_TRANS_TITLE_MS, quad_ease_out);
    /* 标签浮入 */
    anim_start(&state->icon_trans_label_y, 0, 64, 0, 60,
               ICON_TRANS_LABEL_MS, quad_ease_out);
    /* 进度条展开 */
    if (n > 1) {
        int16_t targ = (int16_t)(0 + 1) * 124 / (int16_t)n;
        anim_start(&state->prog_anim, 0, 0, targ, 0, ICON_TRANS_PROG_MS, quad_ease_out);
        state->prog_target = targ;
    }

    /* 图标依次从中央飞向各自位置 (首项最快) */
    int16_t origin_x = FRAME_CENTER_X - (int16_t)iw / 2;
    for (uint8_t i = 0; i < n && i < MENU_MAX_ITEMS; i++) {
        int16_t target_x = (int16_t)(FRAME_CENTER_X + ((int16_t)i - 0) * step)
                           - (int16_t)newp->items[i].icon.w / 2;
        uint32_t dur = (uint32_t)ICON_TRANS_ICON_BASE + (uint32_t)i * ICON_TRANS_ICON_STEP;
        anim_start(&state->icon_trans_item_x[i], origin_x, 0, target_x, 0, dur, quad_ease_out);
    }
}

/* ======== enter / back ======== */

bool menu_key_enter(menu_state_t *state) {
    if (state->trans != TRANS_NONE) return false;
    const menu_item_t *item = &state->current->items[state->selected];
    if (item->action) { item->action(); return true; }
    if (item->submenu) {
        state->trans_old     = state->current;
        state->trans_old_sel = state->selected;
        state->current       = item->submenu;
        state->selected      = 0;
        state->trans         = TRANS_OLD_OUT;
        trans_start_old(state, 7);
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
        trans_start_old(state, 7);
        return true;
    }
    return false;
}
