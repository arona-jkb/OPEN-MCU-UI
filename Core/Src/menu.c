#include "menu.h"

#define BOX_PAD_X    5
#define BOX_PAD_Y    1
#define BOX_RADIUS   3

#define BAR_ANIM_MS      200
#define SCROLL_ANIM_MS   350

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
}

void menu_render(u8g2_t *u8g2, menu_state_t *state) {
    int16_t scroll = state->scroll_anim.cur_y;
    const menu_page_t *page = state->current;
    uint8_t sel = state->selected;

    /* font & metrics */
    u8g2_SetFont(u8g2, u8g2_font_helvB10_tf);
    int16_t ascent  = u8g2_GetAscent(u8g2);
    int16_t box_h   = MENU_LINE_HEIGHT;

    /* ----- compute bar target position & width ----- */
    int16_t item_y     = VISIBLE_TOP + (int16_t)sel * MENU_LINE_HEIGHT - scroll + ascent;
    int16_t targ_box_y = item_y - ascent - BOX_PAD_Y;
    u8g2_uint_t str_w  = u8g2_GetStrWidth(u8g2, page->items[sel].name);
    int16_t targ_box_w = (int16_t)str_w + BOX_PAD_X * 2;

    /* clamp to visible boundaries */
    if (targ_box_y < VISIBLE_TOP + 1)        targ_box_y = VISIBLE_TOP + 1;
    if (targ_box_y > 64 - box_h)             targ_box_y = 64 - box_h;

    /* start bar animation if target changed */
    if (targ_box_y != state->bar_target_y || targ_box_w != state->bar_target_w) {
        int16_t start_y = state->bar_anim.cur_y;
        int16_t start_w = state->bar_anim.cur_x;
        /* first frame: jump to correct position */
        if (state->bar_target_y < 0) {
            start_y = targ_box_y;
            start_w = targ_box_w;
        }
        anim_start(&state->bar_anim, start_w, start_y,
                   targ_box_w, targ_box_y,
                   BAR_ANIM_MS, quad_ease_out);
        state->bar_target_y = targ_box_y;
        state->bar_target_w = targ_box_w;
    }

    int16_t bar_y = state->bar_anim.cur_y;
    int16_t bar_w = state->bar_anim.cur_x;

    /* ----- non-selected items (pass 1, behind box) ----- */
    for (uint8_t i = 0; i < page->count; i++) {
        if (i == sel) continue;
        int16_t y = VISIBLE_TOP + (int16_t)i * MENU_LINE_HEIGHT - scroll + ascent;
        if (y < VISIBLE_TOP || y > 65) continue;
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawStr(u8g2, 4 + BOX_PAD_X, y, page->items[i].name);
    }

    /* ----- selected item box + text (pass 2) ----- */
    {
        int16_t bx         = 2;
        bool    bar_moving = (state->bar_anim.state == ANIM_PLAYING);

        /* box at animated position */
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawRBox(u8g2, bx, bar_y, bar_w, box_h, BOX_RADIUS);

        /* text at natural position — invert only when bar has arrived */
        if (bar_moving) {
            u8g2_DrawStr(u8g2, bx + BOX_PAD_X, item_y, page->items[sel].name);
        } else {
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawStr(u8g2, bx + BOX_PAD_X, item_y, page->items[sel].name);
            u8g2_SetDrawColor(u8g2, 1);
        }
    }

    /* ----- title bar (drawn last to mask scrolled text) ----- */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, 0, 0, 128, VISIBLE_TOP - 1);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    u8g2_DrawStr(u8g2, 2, VISIBLE_TOP - 3, page->title);
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawHLine(u8g2, 0, VISIBLE_TOP - 1, 128);
}

bool menu_key_up(menu_state_t *state) {
    if (state->selected > 0) {
        state->selected--;
        start_scroll(state);
        return true;
    }
    return false;
}

bool menu_key_down(menu_state_t *state) {
    if (state->selected + 1 < state->current->count) {
        state->selected++;
        start_scroll(state);
        return true;
    }
    return false;
}

bool menu_key_enter(menu_state_t *state) {
    const menu_item_t *item = &state->current->items[state->selected];
    if (item->action) {
        item->action();
        return true;
    }
    if (item->submenu) {
        state->current = item->submenu;
        state->selected = 0;
        anim_set_position(&state->scroll_anim, 0, 0);
        state->scroll_target = 0;
        /* force bar re-init on next render */
        state->bar_target_y = -1;
        state->bar_target_w = -1;
        return true;
    }
    return false;
}

bool menu_key_back(menu_state_t *state) {
    if (state->current->parent) {
        state->current = state->current->parent;
        state->selected = 0;
        anim_set_position(&state->scroll_anim, 0, 0);
        state->scroll_target = 0;
        state->bar_target_y = -1;
        state->bar_target_w = -1;
        return true;
    }
    return false;
}
