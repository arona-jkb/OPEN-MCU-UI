#include "menu.h"
#include "ui_timing.h"

#define BOX_PAD_X    5
#define BOX_PAD_Y    1
#define BOX_RADIUS   2

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

static void start_scroll(menu_state_t *state) {
    int16_t target = calc_scroll_target(state);
    state->scroll_target = target;
    anim_start(&state->scroll_anim,
               0, state->scroll_anim.cur_y,
               0, target,
               SCROLL_ANIM_MS, quad_ease_out);
}

void menu_init(menu_state_t *state, const menu_page_t *root) {
    state->current = root;
    state->selected = 0;
    state->scroll_target = 0;
    anim_init(&state->scroll_anim);
    anim_set_position(&state->scroll_anim, 0, 0);
    anim_init(&state->bar_anim);
    state->bar_target_y = -1;
    state->bar_target_w = -1;
    anim_init(&state->prog_anim);
    state->prog_target = -1;
    state->trans = TRANS_NONE;
    anim_init(&state->trans_anim);
}

void menu_update(menu_state_t *state) {
    if (state->trans == TRANS_NONE) return;
    if (state->trans_anim.state == ANIM_FINISHED ||
        state->trans_anim.state == ANIM_IDLE) {
        state->trans = TRANS_NONE;
        anim_set_position(&state->scroll_anim, 0, 0);
        state->scroll_target = 0;
        state->bar_target_y = -1;
        state->bar_target_w = -1;
        state->prog_target  = -1;
    }
}

/* ======== transition render ======== */

static void render_page_slide(u8g2_t *u8g2, const menu_page_t *page,
                              int16_t title_y, int16_t text_shift,
                              int16_t ascent, int16_t box_h) {
    int16_t ttl = VISIBLE_TOP;

    /* items — normal text */
    u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
    u8g2_SetDrawColor(u8g2, 1);
    for (uint8_t i = 0; i < page->count; i++) {
        int16_t y = ttl + (int16_t)i * MENU_LINE_HEIGHT + ascent + text_shift;
        if (y < 2 || y > 65) continue;
        u8g2_DrawStr(u8g2, 4 + BOX_PAD_X, y, page->items[i].name);
    }

    /* sliding title bar — black fill + white text + separator at title_y */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawBox(u8g2, 0, title_y - ttl + 1, 128, ttl);
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawHLine(u8g2, 0, title_y, 128);
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    if (title_y >= 3 && title_y <= 63) {
        u8g2_DrawStr(u8g2, 2, title_y - 3, page->title);
    }
}

/* ======== main render ======== */

void menu_render(u8g2_t *u8g2, menu_state_t *state) {
    u8g2_SetFontMode(u8g2, 1);

    if (state->trans != TRANS_NONE) {
        /* ---- transition ---- */
        int16_t p   = state->trans_anim.cur_y;        /* 0 → 100 */
        int16_t ttl = VISIBLE_TOP;                    /* = 12 */

        int16_t old_title_y = (ttl - 1) - (p * (ttl * 2 - 1)) / 100;
        int16_t old_text_sh =             (p * 64) / 100;    /*  0 → +64  down  */
        int16_t new_title_y = -ttl + (p * (ttl * 2 - 1)) / 100;
        int16_t new_text_sh =  64 - (p * 64) / 100;         /* +64 → 0   up    */

        u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);
        int16_t ascent = u8g2_GetAscent(u8g2);

        render_page_slide(u8g2, state->trans_old, old_title_y, old_text_sh, ascent, MENU_LINE_HEIGHT);
        render_page_slide(u8g2, state->current,   new_title_y, new_text_sh, ascent, MENU_LINE_HEIGHT);

        /* selection bar — follows first item of NEW page (drawn last) */
        {
            u8g2_SetFont(u8g2, u8g2_font_helvB10_tr);  /* restore after title font change */

            int16_t item_y  = ttl + ascent + new_text_sh;
            int16_t box_top = item_y - ascent - BOX_PAD_Y;
            if (box_top < ttl + 1) box_top = ttl + 1;
            if (box_top > 64 - MENU_LINE_HEIGHT) box_top = 64 - MENU_LINE_HEIGHT;

            int16_t bx = 2;
            u8g2_uint_t str_w = u8g2_GetStrWidth(u8g2, state->current->items[0].name);
            int16_t bw = (int16_t)str_w + BOX_PAD_X * 2;

            u8g2_SetDrawColor(u8g2, 2);
            u8g2_DrawRBox(u8g2, bx, box_top, bw, MENU_LINE_HEIGHT, BOX_RADIUS);
        }

        /* no extra separator — render_page_slide already draws it */

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

        /* ---- pass 1: all items normal text ---- */
        for (uint8_t i = 0; i < page->count; i++) {
            int16_t y = VISIBLE_TOP + (int16_t)i * MENU_LINE_HEIGHT - scroll + ascent;
            if (y < VISIBLE_TOP || y > 65) continue;
            u8g2_SetDrawColor(u8g2, 1);
            u8g2_DrawStr(u8g2, 4 + BOX_PAD_X, y, page->items[i].name);
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

bool menu_key_enter(menu_state_t *state) {
    if (state->trans != TRANS_NONE) return false;
    const menu_item_t *item = &state->current->items[state->selected];
    if (item->action) {
        item->action();
        return true;
    }
    if (item->submenu) {
        state->trans_old = state->current;
        state->current   = item->submenu;
        state->selected  = 0;
        state->trans     = TRANS_ENTER;
        anim_start(&state->trans_anim, 0, 0, 0, 100, TRANS_MS, quad_ease_out);
        return true;
    }
    return false;
}

bool menu_key_back(menu_state_t *state) {
    if (state->trans != TRANS_NONE) return false;
    if (state->current->parent) {
        state->trans_old = state->current;
        state->current   = state->current->parent;
        state->selected  = 0;
        state->trans     = TRANS_BACK;
        anim_start(&state->trans_anim, 0, 0, 0, 100, TRANS_MS, quad_ease_out);
        return true;
    }
    return false;
}
