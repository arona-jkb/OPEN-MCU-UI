/**
 * 菜单模块实现
 *
 * 核心功能:
 *   - 多级层次菜单, 支持页面切换动画
 *   - 边界触发式滚动 (选中项碰顶/触底时整体文字才滚动)
 *   - 选择条独立动画 (位置 + 宽度, 由 bar_anim 驱动)
 *   - 右侧滚动进度指示条 (由 prog_anim 驱动)
 *   - XOR 模式选择框 (颜色 2), 与菜单文字自然叠加
 *   - 图标菜单入场动画 (标题/标签浮入, 图标依次飞入, 进度条展开)
 */
#include "menu.h"
#include "ui_timing.h"

#define BOX_PAD_X    5                    /* 选择框左右内边距      */
#define BOX_PAD_Y    1                    /* 选择框上下内边距      */
#define BOX_RADIUS   1                    /* 选择框圆角半径        */

#define BAR_MAX_W      122               /* 选择条最大宽度: 比右侧进度条(x=125)小1px */
#define TEXT_START_X   (4 + BOX_PAD_X)    /* 菜单文字起始 X 坐标 = 9                    */
#define TEXT_MAX_END   (2 + BAR_MAX_W - BOX_PAD_X) /* 文字可见右边界 = 119           */
#define TEXT_VISIBLE_W (TEXT_MAX_END - TEXT_START_X) /* 可见文字宽度 = 110            */
#define ICON_GAP      8                     /* 图标之间水平间距 (像素) */
#define ICON_FRAME_GAP 4                     /* 选中外框与图标间距       */
#define ICON_BRACKET_LEN 6                   /* 直角拐角臂长            */
#define ICON_BRACKET_THICK 2                 /* 拐角线条粗细 (像素)     */

/* 绘制直角拐角选中框 (XOR 颜色2) */
#define DRAW_ICON_BRACKETS(u8g2, fx, fy, fw, fh) do {                   \
    int16_t _bx = (int16_t)(fx), _by = (int16_t)(fy);                   \
    int16_t _bw = (int16_t)(fw), _bh = (int16_t)(fh);                   \
    int16_t _bl = ICON_BRACKET_LEN, _bt = ICON_BRACKET_THICK;           \
    /* 左上 横+竖 */                                                       \
    u8g2_DrawBox(u8g2, _bx,              _by,              _bl, _bt);   \
    u8g2_DrawBox(u8g2, _bx,              _by,              _bt, _bl);   \
    /* 右上 横+竖 */                                                       \
    u8g2_DrawBox(u8g2, _bx+_bw-_bl,      _by,              _bl, _bt);   \
    u8g2_DrawBox(u8g2, _bx+_bw-_bt,      _by,              _bt, _bl);   \
    /* 左下 横+竖 */                                                       \
    u8g2_DrawBox(u8g2, _bx,              _by+_bh-_bt,      _bl, _bt);   \
    u8g2_DrawBox(u8g2, _bx,              _by+_bh-_bl,      _bt, _bl);   \
    /* 右下 横+竖 */                                                       \
    u8g2_DrawBox(u8g2, _bx+_bw-_bl,      _by+_bh-_bt,      _bl, _bt);   \
    u8g2_DrawBox(u8g2, _bx+_bw-_bt,      _by+_bh-_bl,      _bt, _bl);   \
} while(0)

/* 图标菜单入场动画参数 */
#define ICON_TRANS_TITLE_MS 400            /* 标题栏入场时长         */
#define ICON_TRANS_LABEL_MS 400            /* 标签入场时长           */
#define ICON_TRANS_ICON_BASE 200           /* 首项图标飞入时长       */
#define ICON_TRANS_ICON_STEP 80            /* 每项递增延迟          */
#define ICON_TRANS_PROG_MS   400           /* 进度条展开时长         */

#define VISIBLE_TOP    MENU_TITLE_HEIGHT
#define VISIBLE_BOTTOM (64 - MENU_LINE_HEIGHT)
#define TEXT_TOP_PAD 3                 /* 标题分隔线下留白, 确保首项选择框对齐 */

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

/* 前向声明 — 页面切换函数 */
static void trans_start_old(menu_state_t *state, int16_t ascent);
static void trans_start_new_text(menu_state_t *state, int16_t ascent);
static void icon_trans_start(menu_state_t *state);
static void icon_trans_start_exit(menu_state_t *state);

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
    anim_init(&state->icon_frame_anim);
    anim_set_position(&state->icon_frame_anim, 0, 0);
    state->icon_frame_target = -1;
    anim_init(&state->icon_scroll_anim);
    anim_set_position(&state->icon_scroll_anim, 0, 0);
    state->icon_scroll_target = -1;
    /* 图标入场过渡 */
    anim_init(&state->icon_trans_title_y);
    anim_set_position(&state->icon_trans_title_y, 0, 0);
    anim_init(&state->icon_trans_label_y);
    anim_set_position(&state->icon_trans_label_y, 0, 0);
    for (int i = 0; i < MENU_MAX_ITEMS; i++) {
        anim_init(&state->icon_trans_item_x[i]);
        anim_set_position(&state->icon_trans_item_x[i], 0, 0);
    }
    state->icon_trans_step = 0;
    state->icon_trans_t0   = 0;
    /* 标签切换动画 */
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
        if (state->trans_old->style == MENU_ICON) {
            /* 图标退出: 等最后一个图标完成 */
            uint8_t n = state->trans_old->count;
            if (n > 0) {
                uint8_t last = n - 1;
                if (last >= MENU_MAX_ITEMS) last = MENU_MAX_ITEMS - 1;
                if (state->icon_trans_item_x[last].state == ANIM_FINISHED ||
                    state->icon_trans_item_x[last].state == ANIM_IDLE) goto icon_out_done;
            } else {
                if (state->icon_trans_title_y.state == ANIM_FINISHED ||
                    state->icon_trans_title_y.state == ANIM_IDLE) goto icon_out_done;
            }
        } else {
            /* 文字菜单退出 */
            if (state->title_old.state == ANIM_FINISHED ||
                state->title_old.state == ANIM_IDLE) goto text_out_done;
        }
        return;

    icon_out_done:
    text_out_done:
        /* 退出完成 → 启动新页进入 */
        state->trans = TRANS_NEW_IN;
        if (state->current->style == MENU_ICON)
            icon_trans_start(state);
        else
            trans_start_new_text(state, 7);
    } else if (state->trans == TRANS_NEW_IN) {
        if (state->current->style == MENU_ICON) {
            /* 图标入场: 所有图标飞入完成后结束过渡 */
            uint8_t n = state->current->count;
            if (n > 0) {
                /* 等最后一个图标完成 */
                uint8_t last = n - 1;
                if (last >= MENU_MAX_ITEMS) last = MENU_MAX_ITEMS - 1;
                if (state->icon_trans_item_x[last].state == ANIM_FINISHED ||
                    state->icon_trans_item_x[last].state == ANIM_IDLE) {
                    state->trans = TRANS_NONE;
                    anim_set_position(&state->scroll_anim, 0, 0);
                    state->scroll_target = 0;
                    state->bar_target_y = -1;
                    state->bar_target_w = -1;
                    state->text_scroll_target = -1;
                    state->icon_frame_target = -1;
                    state->icon_scroll_target = -1;
                    state->prog_target  = -1;
                    state->icon_label_old_name = NULL;
                    state->icon_label_new_name = NULL;
                    state->icon_label_phase    = 0;
                }
            } else {
                /* 空页: 等标题完成 */
                if (state->icon_trans_title_y.state == ANIM_FINISHED ||
                    state->icon_trans_title_y.state == ANIM_IDLE) {
                    state->trans = TRANS_NONE;
                    state->icon_frame_target = -1;
                    state->icon_scroll_target = -1;
                    state->prog_target  = -1;
                }
            }
        } else {
            /* 文字菜单过渡 */
            if (state->title_new.state == ANIM_FINISHED ||
                state->title_new.state == ANIM_IDLE) {
                state->trans = TRANS_NONE;
                anim_set_position(&state->scroll_anim, 0, 0);
                state->scroll_target = 0;
                state->bar_target_y = -1;
                state->bar_target_w = -1;
                state->text_scroll_target = -1;
                state->icon_frame_target = -1;
                state->icon_scroll_target = -1;
                state->prog_target  = -1;
            }
        }
    }
}

/* ======== main render ======== */

void menu_render(u8g2_t *u8g2, menu_state_t *state) {
    u8g2_SetFontMode(u8g2, 1);

    if (state->trans == TRANS_OLD_OUT) {
        const menu_page_t *oldp = state->trans_old;

        if (oldp->style == MENU_ICON) {
            /* ================================================================
             *  图标菜单退场 (入场反向)
             * ================================================================ */
            uint8_t n = oldp->count;
            uint8_t os = state->trans_old_sel;
            const menu_item_t *sel_item = &oldp->items[os];
            uint8_t iw = sel_item->icon.w;
            uint8_t ih = sel_item->icon.h;
            int16_t frame_pad = ICON_FRAME_GAP;
            int16_t total_gap  = (int16_t)(n - 1) * ICON_GAP;
            int16_t slot_w     = (TEXT_VISIBLE_W - total_gap) / (int16_t)n;
            if (slot_w < (int16_t)iw + frame_pad * 2) slot_w = (int16_t)iw + frame_pad * 2;
            int16_t icon_y     = VISIBLE_TOP + 8;

            /* 标题栏 (动画 Y) */
            {
                int16_t ttl_y = state->icon_trans_title_y.cur_y;
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawBox(u8g2, 0, ttl_y - VISIBLE_TOP + 1, 128, VISIBLE_TOP);
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
                u8g2_uint_t ttw = u8g2_GetStrWidth(u8g2, oldp->title);
                int16_t ttx = (int16_t)(128 - ttw) / 2;
                if (ttx < 0) ttx = 0;
                u8g2_DrawStr(u8g2, (u8g2_uint_t)ttx, (u8g2_uint_t)(ttl_y - 3), oldp->title);
                if (n > 1) {
                    int16_t pw = state->prog_anim.cur_x;
                    if (pw > 0) u8g2_DrawBox(u8g2, 0, VISIBLE_TOP, (u8g2_uint_t)pw, 3);
                }
            }

            /* 选中框 (XOR) + 图标 (z-order: 末项→首项) */
            {
                int16_t frame_x = state->icon_frame_anim.cur_x;
                int16_t frame_y = icon_y - frame_pad;
                int16_t frame_w = (int16_t)iw + (int16_t)frame_pad * 2;
                int16_t frame_h = (int16_t)ih + (int16_t)frame_pad * 2;
                u8g2_SetDrawColor(u8g2, 2);
                DRAW_ICON_BRACKETS(u8g2, frame_x, frame_y, frame_w, frame_h);

                u8g2_SetDrawColor(u8g2, 1);
                for (int8_t i = (int8_t)(n - 1); i >= 0; i--) {
                    if (i >= MENU_MAX_ITEMS) continue;
                    int16_t ix = state->icon_trans_item_x[i].cur_x;
                    u8g2_DrawXBMP(u8g2, (u8g2_uint_t)ix, (u8g2_uint_t)icon_y,
                                  oldp->items[i].icon.w, oldp->items[i].icon.h,
                                  oldp->items[i].icon.bitmap);
                }
            }

            /* 标签 */
            {
                int16_t label_y = state->icon_trans_label_y.cur_y;
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
                u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, sel_item->name);
                int16_t tx = (int16_t)(128 - tw) / 2;
                if (tx < 0) tx = 0;
                u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, (u8g2_uint_t)label_y, sel_item->name);
            }

        } else {
        /* ---- 文字菜单退出 (全部向上收缩至 y=-12) ---- */
        int16_t ttl = VISIBLE_TOP;

        /* 旧页项 */
        u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
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

        /* 旧页选择条 */
        u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
        if (oldp->count > 0) {
            uint8_t os = state->trans_old_sel;
            int16_t ascent2 = u8g2_GetAscent(u8g2);
            int16_t box_top = state->items_old[os].cur_y - ascent2 - BOX_PAD_Y;
            u8g2_uint_t sw = u8g2_GetStrWidth(u8g2, oldp->items[os].name);
            u8g2_SetDrawColor(u8g2, 2);
            u8g2_DrawRBox(u8g2, 2, box_top, (int16_t)sw + BOX_PAD_X * 2, MENU_LINE_HEIGHT, BOX_RADIUS);
        }

        }  /* end MENU_TEXT old-out branch */

    } else if (state->trans == TRANS_NEW_IN) {
        const menu_page_t *newp = state->current;

        if (newp->style == MENU_ICON) {
            /* ================================================================
             *  图标菜单入场过渡
             *  - 标题从顶端滑入 (居中)
             *  - 进度条从 0 展开
             *  - 图标依次从中心飞入 (z-order: 首项在最上层)
             *  - 选中框独立移动到首项
             *  - 标签从底部浮入
             * ================================================================ */
            uint8_t n = newp->count;
            const menu_item_t *sel_item = &newp->items[0];  /* 入场时 sel=0 */
            uint8_t iw = sel_item->icon.w;
            uint8_t ih = sel_item->icon.h;
            int16_t frame_pad = ICON_FRAME_GAP;
            int16_t total_gap  = (int16_t)(n - 1) * ICON_GAP;
            int16_t slot_w     = (TEXT_VISIBLE_W - total_gap) / (int16_t)n;
            if (slot_w < (int16_t)iw + frame_pad * 2) slot_w = (int16_t)iw + frame_pad * 2;
            int16_t icon_y     = VISIBLE_TOP + 8;

            /* ---- 标题栏 (黑底 + 居中标题, 动画 Y) ---- */
            {
                int16_t ttl_y = state->icon_trans_title_y.cur_y;
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawBox(u8g2, 0, ttl_y - VISIBLE_TOP + 1, 128, VISIBLE_TOP);
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
                u8g2_uint_t ttw = u8g2_GetStrWidth(u8g2, newp->title);
                int16_t ttx = (int16_t)(128 - ttw) / 2;
                if (ttx < 0) ttx = 0;
                u8g2_DrawStr(u8g2, (u8g2_uint_t)ttx, (u8g2_uint_t)(ttl_y - 3), newp->title);
                /* 进度条 */
                if (n > 1) {
                    int16_t pw = state->prog_anim.cur_x;
                    if (pw > 0) u8g2_DrawBox(u8g2, 0, VISIBLE_TOP, (u8g2_uint_t)pw, 3);
                }
            }

            /* ---- 图标 + 选中框 (z-order: 末项→首项, 框在首项上) ---- */
            {
                /* 首项选中框 (XOR) */
                int16_t sel_target_cx = TEXT_START_X + (int16_t)0 * (slot_w + ICON_GAP) + slot_w / 2;
                int16_t sel_target_ix = sel_target_cx - (int16_t)iw / 2;
                int16_t frame_target  = sel_target_ix - frame_pad;
                {
                    int16_t frame_x = state->icon_frame_anim.cur_x;
                    int16_t frame_y = icon_y - frame_pad;
                    int16_t frame_w = (int16_t)iw + frame_pad * 2;
                    int16_t frame_h = (int16_t)ih + frame_pad * 2;
                    u8g2_SetDrawColor(u8g2, 2);
                    DRAW_ICON_BRACKETS(u8g2, frame_x, frame_y, frame_w, frame_h);
                }

                /* 图标: 从末项到首项绘制 (首项在最上层) */
                /* 非首项的图标不绘制到框内, 框会 XOR 盖在上面 */
                u8g2_SetDrawColor(u8g2, 1);
                for (int8_t i = (int8_t)(n - 1); i >= 0; i--) {
                    if (i >= MENU_MAX_ITEMS) continue;
                    int16_t ix = state->icon_trans_item_x[i].cur_x;
                    if (i == 0) {
                        /* 首项: 确保不偏离框内 (即使动画未到, 也与框对齐) */
                        if (ix < frame_target + frame_pad)
                            ix = frame_target + frame_pad;
                    }
                    u8g2_DrawXBMP(u8g2, (u8g2_uint_t)ix, (u8g2_uint_t)icon_y,
                                  newp->items[i].icon.w, newp->items[i].icon.h,
                                  newp->items[i].icon.bitmap);
                }
            }

            /* ---- 标签: 从底部浮入 ---- */
            {
                int16_t label_y = state->icon_trans_label_y.cur_y;
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
                u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, sel_item->name);
                int16_t tx = (int16_t)(128 - tw) / 2;
                if (tx < 0) tx = 0;
                u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, (u8g2_uint_t)label_y, sel_item->name);
            }
        } else {
            /* ---- 文字菜单新页进入 (全部从 y=-12 向下展开) ---- */
            int16_t ttl = VISIBLE_TOP;
            int16_t box_h = MENU_LINE_HEIGHT;

            u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
            int16_t ascent = u8g2_GetAscent(u8g2);

            u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
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

            /* 选择条 */
            u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
            if (newp->count > 0) {
                int16_t box_top = state->items_new[0].cur_y - ascent - BOX_PAD_Y;
                u8g2_uint_t sw = u8g2_GetStrWidth(u8g2, newp->items[0].name);
                u8g2_SetDrawColor(u8g2, 2);
                u8g2_DrawRBox(u8g2, 2, box_top, (int16_t)sw + BOX_PAD_X * 2, box_h, BOX_RADIUS);
            }
        }

    } else {
        /* ---- normal render ---- */
        int16_t scroll = state->scroll_anim.cur_y;
        const menu_page_t *page = state->current;
        uint8_t sel = state->selected;

        /* ================================================================
         *  图标菜单 (MENU_ICON): 水平排列图标 + 下方文字
         * ================================================================ */
        if (page->style == MENU_ICON) {
            uint8_t n = page->count;
            if (n == 0) goto draw_title;

            const menu_item_t *sel_item = &page->items[sel];
            uint8_t iw = sel_item->icon.w;
            uint8_t ih = sel_item->icon.h;
            int16_t frame_pad  = ICON_FRAME_GAP;
            int16_t frame_half = (int16_t)iw / 2 + frame_pad;

            /* 槽位宽度 */
            int16_t total_gap = (int16_t)(n - 1) * ICON_GAP;
            int16_t slot_w = (TEXT_VISIBLE_W - total_gap) / (int16_t)n;
            if (slot_w < (int16_t)iw + frame_pad * 2) slot_w = (int16_t)iw + frame_pad * 2;

            int16_t icon_y = VISIBLE_TOP + 8;

            /* ---- 1.  水平滚动目标 ---- */
            int16_t sel_abs_cx = TEXT_START_X + (int16_t)sel * (slot_w + ICON_GAP) + slot_w / 2;
            int16_t cur_s      = state->icon_scroll_anim.cur_x;
            int16_t targ_s     = cur_s;

            if (sel_abs_cx - frame_half - cur_s < TEXT_START_X)
                targ_s = cur_s - (TEXT_START_X - (sel_abs_cx - frame_half - cur_s));
            if (sel_abs_cx + frame_half - cur_s > TEXT_MAX_END)
                targ_s = cur_s + ((sel_abs_cx + frame_half - cur_s) - TEXT_MAX_END);
            if (targ_s < 0) targ_s = 0;

            if (targ_s != state->icon_scroll_target) {
                int16_t start_s = cur_s;
                if (state->icon_scroll_target < 0) start_s = targ_s;
                anim_start(&state->icon_scroll_anim, start_s, 0,
                           targ_s, 0, SCROLL_ANIM_MS, quad_ease_out);
                state->icon_scroll_target = targ_s;
            }
            int16_t scroll_offs = state->icon_scroll_anim.cur_x;

            /* ---- 2.  选中框目标 (用 scroll 目标值, 独立动画) ---- */
            {
                int16_t frame_final = sel_abs_cx - targ_s - (int16_t)iw / 2 - frame_pad;
                if (frame_final < TEXT_START_X)
                    frame_final = TEXT_START_X;
                if (frame_final + (int16_t)iw + frame_pad * 2 > TEXT_MAX_END)
                    frame_final = TEXT_MAX_END - (int16_t)iw - frame_pad * 2;

                if (frame_final != state->icon_frame_target) {
                    int16_t start_x = state->icon_frame_anim.cur_x;
                    if (state->icon_frame_target < 0) start_x = frame_final;
                    anim_start(&state->icon_frame_anim, start_x, 0,
                               frame_final, 0, BAR_ANIM_MS, quad_ease_out);
                    state->icon_frame_target = frame_final;
                }
            }

            /* ---- pass 1: 所有图标统一按 scroll 偏移绘制 (颜色 1) ---- */
            u8g2_SetDrawColor(u8g2, 1);
            for (uint8_t i = 0; i < n; i++) {
                const menu_item_t *item = &page->items[i];
                int16_t abs_cx = TEXT_START_X + (int16_t)i * (slot_w + ICON_GAP) + slot_w / 2;
                int16_t ix = abs_cx - scroll_offs - (int16_t)item->icon.w / 2;
                if (ix + (int16_t)item->icon.w < TEXT_START_X || ix > TEXT_MAX_END) continue;
                u8g2_DrawXBMP(u8g2, (u8g2_uint_t)ix, (u8g2_uint_t)icon_y,
                              item->icon.w, item->icon.h, item->icon.bitmap);
            }

            /* ---- pass 2: XOR 直角拐角选中框 (颜色 2) ---- */
            {
                int16_t frame_x = state->icon_frame_anim.cur_x;
                int16_t frame_y = icon_y - frame_pad;
                int16_t frame_w = (int16_t)iw + frame_pad * 2;
                int16_t frame_h = (int16_t)ih + frame_pad * 2;
                u8g2_SetDrawColor(u8g2, 2);
                DRAW_ICON_BRACKETS(u8g2, frame_x, frame_y, frame_w, frame_h);
            }

            /* ---- 文字标签: 屏幕下方居中 (阶段1:旧下沉 → 阶段2:新上浮) ---- */
            {
                u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
                u8g2_SetDrawColor(u8g2, 1);

                if (state->icon_label_phase == 1) {
                    /* 阶段1: 仅绘制旧标签 (下沉 60→64) */
                    int16_t old_y = state->icon_label_old_y.cur_y;
                    u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, state->icon_label_old_name);
                    int16_t tx = (int16_t)(128 - tw) / 2;
                    if (tx < 0) tx = 0;
                    u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, (u8g2_uint_t)old_y, state->icon_label_old_name);

                    if (state->icon_label_old_y.state == ANIM_FINISHED ||
                        state->icon_label_old_y.state == ANIM_IDLE) {
                        /* 阶段1完成 → 启动阶段2 */
                        anim_start(&state->icon_label_new_y, 0, 64, 0, 60,
                                   BAR_ANIM_MS, quad_ease_out);
                        state->icon_label_phase = 2;
                    }
                } else if (state->icon_label_phase == 2) {
                    /* 阶段2: 仅绘制新标签 (上浮 64→60) */
                    int16_t new_y = state->icon_label_new_y.cur_y;
                    u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, state->icon_label_new_name);
                    int16_t tx = (int16_t)(128 - tw) / 2;
                    if (tx < 0) tx = 0;
                    u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, (u8g2_uint_t)new_y, state->icon_label_new_name);

                    if (state->icon_label_new_y.state == ANIM_FINISHED ||
                        state->icon_label_new_y.state == ANIM_IDLE) {
                        /* 阶段2完成 → 恢复空闲 */
                        state->icon_label_phase    = 0;
                        state->icon_label_old_name = NULL;
                        state->icon_label_new_name = NULL;
                    }
                } else {
                    /* 空闲: 正常绘制当前选中项标签 */
                    u8g2_uint_t tw = u8g2_GetStrWidth(u8g2, sel_item->name);
                    int16_t tx = (int16_t)(128 - tw) / 2;
                    if (tx < 0) tx = 0;
                    u8g2_DrawStr(u8g2, (u8g2_uint_t)tx, 60, sel_item->name);
                }
            }

            /* ---- 标题栏进度条 ---- */
            if (n > 1) {
                int16_t targ = (int16_t)(sel + 1) * 124 / (int16_t)n;
                if (targ != state->prog_target) {
                    int16_t start_w = state->prog_anim.cur_x;
                    if (state->prog_target < 0) start_w = targ;
                    anim_start(&state->prog_anim, start_w, 0,
                               targ, 0, BAR_ANIM_MS, quad_ease_out);
                    state->prog_target = targ;
                }
            }

            goto draw_title;
        }

        /* ================================================================
         *  文字菜单 (MENU_TEXT): 原有垂直文字逻辑
         * ================================================================ */

        u8g2_SetFont(u8g2, u8g2_font_helvB08_tr);
        int16_t ascent = u8g2_GetAscent(u8g2);
        int16_t box_h  = MENU_LINE_HEIGHT;

        /* bar target */
        int16_t item_y     = VISIBLE_TOP + TEXT_TOP_PAD + (int16_t)sel * MENU_LINE_HEIGHT - scroll + ascent;
        int16_t targ_box_y = item_y - ascent - BOX_PAD_Y;
        u8g2_uint_t str_w = u8g2_GetStrWidth(u8g2, page->items[sel].name);
        int16_t targ_box_w = (int16_t)str_w + BOX_PAD_X * 2;

        if (targ_box_w > BAR_MAX_W) targ_box_w = BAR_MAX_W;

        if (targ_box_y < VISIBLE_TOP)        targ_box_y = VISIBLE_TOP;
        if (targ_box_y > 64 - box_h)         targ_box_y = 64 - box_h;

        if (targ_box_y != state->bar_target_y || targ_box_w != state->bar_target_w) {
            int16_t start_y = state->bar_anim.cur_y;
            int16_t start_w = state->bar_anim.cur_x;
            if (state->bar_target_y < 0) { start_y = targ_box_y; start_w = targ_box_w; }
            anim_start(&state->bar_anim, start_w, start_y,
                       targ_box_w, targ_box_y, BAR_ANIM_MS, quad_ease_out);
            state->bar_target_y = targ_box_y;
            state->bar_target_w = targ_box_w;
        }

        /* ---- 选中项文字水平滚动 ---- */
        {
            int16_t text_overflow = (int16_t)str_w - TEXT_VISIBLE_W;
            if (text_overflow > 0) {
                int16_t targ = text_overflow;
                if (targ != state->text_scroll_target) {
                    int16_t start_s = state->text_scroll_anim.cur_x;
                    if (state->text_scroll_target < 0) start_s = 0;
                    uint32_t dur = (uint32_t)text_overflow * 12;
                    if (dur < SCROLL_ANIM_MS) dur = SCROLL_ANIM_MS;
                    anim_start(&state->text_scroll_anim, start_s, 0,
                               targ, 0, dur, linear_ease);
                    state->text_scroll_target = targ;
                }
            } else {
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

        /* ---- pass 1: all items normal text ---- */
        for (uint8_t i = 0; i < page->count; i++) {
            int16_t y = VISIBLE_TOP + TEXT_TOP_PAD + (int16_t)i * MENU_LINE_HEIGHT - scroll + ascent;
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
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawBox(u8g2, 124, VISIBLE_TOP, 1, h);
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawBox(u8g2, 125, VISIBLE_TOP, 3, h);
            }
        }

        /* ---- title bar ---- */
    draw_title:
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, 0, 0, 128, VISIBLE_TOP);
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
        if (page->style == MENU_ICON) {
            /* 图标菜单标题居中 */
            u8g2_uint_t ttw = u8g2_GetStrWidth(u8g2, page->title);
            int16_t ttx = (int16_t)(128 - ttw) / 2;
            if (ttx < 0) ttx = 0;
            u8g2_DrawStr(u8g2, (u8g2_uint_t)ttx, VISIBLE_TOP - 3, page->title);
        } else {
            u8g2_DrawStr(u8g2, 2, VISIBLE_TOP - 3, page->title);
        }
        if (page->style == MENU_ICON && page->count > 1) {
            /* 标题分割线 → 水平进度条 (3px 高) */
            int16_t pw = state->prog_anim.cur_x;
            if (pw > 0) u8g2_DrawBox(u8g2, 0, VISIBLE_TOP, (u8g2_uint_t)pw, 3);
        } else {
            u8g2_DrawHLine(u8g2, 0, VISIBLE_TOP - 1, 128);
        }
    }
}

/* ======== key handlers ======== */

/* 图标菜单选中变化: 触发标签切换动画 */
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
    if (state->selected > 0) {
        state->selected--;
        start_scroll(state);
        return true;
    }
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
    if (state->selected + 1 < state->current->count) {
        state->selected++;
        start_scroll(state);
        return true;
    }
    return false;
}

/* ======== 页面切换辅助 ======== */

static void trans_start_old(menu_state_t *state, int16_t ascent) {
    const menu_page_t *oldp = state->trans_old;

    if (oldp->style == MENU_ICON) {
        icon_trans_start_exit(state);     /* 图标菜单反方向退出 */
        return;
    }

    /* 文字菜单退出 */
    int16_t title_target = VISIBLE_TOP - 1;
    int16_t item_base    = VISIBLE_TOP + TEXT_TOP_PAD;
    int16_t end = -12;
    uint16_t dur = TRANS_MS / 2;

    anim_start(&state->title_old, 0, title_target, 0, end, dur, quad_ease_out);
    for (uint8_t i = 0; i < oldp->count && i < MENU_MAX_ITEMS; i++) {
        int16_t y = item_base + (int16_t)i * MENU_LINE_HEIGHT + ascent;
        anim_start(&state->items_old[i], 0, y, 0, end, dur, quad_ease_out);
    }
    state->bar_target_y = -1;
    state->bar_target_w = -1;
}

/* 文字菜单新页进入 (原有逻辑) */
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
    state->bar_target_y = -1;
    state->bar_target_w = -1;
}

/* 图标菜单退场动画 (入场反向) */
static void icon_trans_start_exit(menu_state_t *state) {
    const menu_page_t *oldp = state->trans_old;
    uint8_t n = oldp->count;
    uint8_t os = state->trans_old_sel;

    int16_t ttl = VISIBLE_TOP;

    /* 标题: 滑出到屏幕上方 */
    anim_start(&state->icon_trans_title_y, 0, ttl - 1, 0, -12,
               ICON_TRANS_TITLE_MS, quad_ease_out);

    /* 标签: 下沉出屏幕底部 */
    anim_start(&state->icon_trans_label_y, 0, 60, 0, 64,
               ICON_TRANS_LABEL_MS, quad_ease_out);

    /* 进度条: 收缩到 0 */
    {
        int16_t start_w = state->prog_anim.cur_x;
        anim_start(&state->prog_anim, start_w, 0, 0, 0,
                   ICON_TRANS_PROG_MS, quad_ease_out);
        state->prog_target = 0;
    }

    /* 布局计算 (使用旧页选中项尺寸) */
    if (n == 0) return;
    const menu_item_t *sel_item = &oldp->items[os];
    uint8_t iw = sel_item->icon.w;
    int16_t frame_pad = ICON_FRAME_GAP;
    int16_t total_gap = (int16_t)(n - 1) * ICON_GAP;
    int16_t slot_w    = (TEXT_VISIBLE_W - total_gap) / (int16_t)n;
    if (slot_w < (int16_t)iw + frame_pad * 2) slot_w = (int16_t)iw + frame_pad * 2;

    /* 飞出目标: 屏幕水平中心 */
    int16_t origin_x = 64 - (int16_t)iw / 2;

    /* 图标依次飞出 (末项先飞, 递增延迟) */
    for (uint8_t i = 0; i < n && i < MENU_MAX_ITEMS; i++) {
        int16_t abs_cx   = TEXT_START_X + (int16_t)i * (slot_w + ICON_GAP) + slot_w / 2;
        int16_t start_x  = abs_cx - (int16_t)oldp->items[i].icon.w / 2;
        uint8_t rev_i    = (uint8_t)(n - 1 - i);       /* 反向索引: 末项先飞 */
        uint32_t dur     = (uint32_t)ICON_TRANS_ICON_BASE + (uint32_t)rev_i * ICON_TRANS_ICON_STEP;
        anim_start(&state->icon_trans_item_x[i], start_x, 0, origin_x, 0, dur, quad_ease_out);
    }

    /* 选中框: 飞到中心 */
    {
        int16_t sel_cx = TEXT_START_X + (int16_t)os * (slot_w + ICON_GAP) + slot_w / 2;
        int16_t frame_start = sel_cx - (int16_t)iw / 2 - frame_pad;
        if (frame_start < TEXT_START_X) frame_start = TEXT_START_X;
        anim_start(&state->icon_frame_anim, frame_start, 0,
                   origin_x - frame_pad, 0, ICON_TRANS_ICON_BASE, quad_ease_out);
        state->icon_frame_target = origin_x - frame_pad;
    }
}

/* 图标菜单入场动画 */
static void icon_trans_start(menu_state_t *state) {
    const menu_page_t *newp = state->current;
    uint8_t n = newp->count;

    int16_t ttl = VISIBLE_TOP;
    int16_t title_end = ttl - 1;

    /* 标题: 从屏幕上方滑入 */
    anim_start(&state->icon_trans_title_y, 0, -12, 0, title_end,
               ICON_TRANS_TITLE_MS, quad_ease_out);

    /* 标签: 从屏幕底部浮入 */
    anim_start(&state->icon_trans_label_y, 0, 64, 0, 60,
               ICON_TRANS_LABEL_MS, quad_ease_out);

    /* 进度条: 从 0 展开到首项位置 */
    if (n > 1) {
        int16_t targ = (int16_t)(0 + 1) * 124 / (int16_t)n;
        anim_start(&state->prog_anim, 0, 0, targ, 0,
                   ICON_TRANS_PROG_MS, quad_ease_out);
        state->prog_target = targ;
    }

    /* 使用首项图标尺寸计算布局 */
    if (n == 0) return;
    const menu_item_t *sel_item = &newp->items[0];
    uint8_t iw = sel_item->icon.w;
    int16_t frame_pad = ICON_FRAME_GAP;
    int16_t total_gap = (int16_t)(n - 1) * ICON_GAP;
    int16_t slot_w    = (TEXT_VISIBLE_W - total_gap) / (int16_t)n;
    if (slot_w < (int16_t)iw + frame_pad * 2) slot_w = (int16_t)iw + frame_pad * 2;

    /* 飞入原点: 屏幕水平中心 */
    int16_t origin_x = 64 - (int16_t)iw / 2;

    /* 图标依次飞入 (递增时长实现 stagger 效果) */
    for (uint8_t i = 0; i < n && i < MENU_MAX_ITEMS; i++) {
        int16_t abs_cx = TEXT_START_X + (int16_t)i * (slot_w + ICON_GAP) + slot_w / 2;
        int16_t target_x = abs_cx - (int16_t)newp->items[i].icon.w / 2;
        uint32_t dur = (uint32_t)ICON_TRANS_ICON_BASE + (uint32_t)i * ICON_TRANS_ICON_STEP;
        anim_start(&state->icon_trans_item_x[i], origin_x, 0, target_x, 0, dur, quad_ease_out);
    }

    /* 选中框: 独立飞到首项 */
    {
        int16_t sel_cx = TEXT_START_X + (int16_t)0 * (slot_w + ICON_GAP) + slot_w / 2;
        int16_t frame_target = sel_cx - (int16_t)iw / 2 - frame_pad;
        if (frame_target < TEXT_START_X) frame_target = TEXT_START_X;
        anim_start(&state->icon_frame_anim, origin_x - frame_pad, 0,
                   frame_target, 0, ICON_TRANS_ICON_BASE, quad_ease_out);
        state->icon_frame_target = frame_target;
    }
}

/* ======== enter / back ======== */

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
