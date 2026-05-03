#include "menu.h"

#define BOX_PAD_X    5
#define BOX_PAD_Y    1
#define BOX_RADIUS   3

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
               200, quad_ease_out);
}

void menu_init(menu_state_t *state, const menu_page_t *root) {
    state->current = root;
    state->selected = 0;
    state->scroll_target = 0;
    anim_init(&state->scroll_anim);
    anim_set_position(&state->scroll_anim, 0, 0);
}

void menu_render(u8g2_t *u8g2, menu_state_t *state) {
    int16_t scroll = state->scroll_anim.cur_y;
    const menu_page_t *page = state->current;
    uint8_t sel = state->selected;

    /* font & metrics (used throughout) */
    u8g2_SetFont(u8g2, u8g2_font_helvB10_tf);
    int16_t ascent = u8g2_GetAscent(u8g2);
    int16_t box_h  = MENU_LINE_HEIGHT;       /* exact fit, no adjacent overlap */

    /* ----- non-selected items (pass 1, behind box) ----- */
    for (uint8_t i = 0; i < page->count; i++) {
        if (i == sel) continue;
        int16_t y = VISIBLE_TOP + (int16_t)i * MENU_LINE_HEIGHT - scroll + ascent;
        if (y < VISIBLE_TOP || y > 65) continue;
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawStr(u8g2, 4 + BOX_PAD_X, y, page->items[i].name);
    }

    /* ----- selected item box + inverted text (pass 2) ----- */
    {
        int16_t item_y  = VISIBLE_TOP + (int16_t)sel * MENU_LINE_HEIGHT - scroll + ascent;

        /* natural box top — only clamp when text is actually off-screen */
        int16_t box_top = item_y - ascent - BOX_PAD_Y;

        if (item_y - ascent < VISIBLE_TOP) {
            /* text top went above title → pin box to top edge */
            box_top = VISIBLE_TOP + 1;
        } else if (item_y + 3 > 64) {
            /* text bottom went below screen → pin box to bottom edge */
            box_top = 64 - box_h;
        }

        /* safety clamps */
        if (box_top < VISIBLE_TOP + 1)       box_top = VISIBLE_TOP + 1;
        if (box_top > 64 - box_h)            box_top = 64 - box_h;

        int16_t bx = 2;
        u8g2_uint_t str_w = u8g2_GetStrWidth(u8g2, page->items[sel].name);
        int16_t bw = (int16_t)str_w + BOX_PAD_X * 2;

        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawRBox(u8g2, bx, box_top, bw, box_h, BOX_RADIUS);
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawStr(u8g2, bx + BOX_PAD_X, item_y, page->items[sel].name);
        u8g2_SetDrawColor(u8g2, 1);
    }

    /* ----- title bar (drawn last to mask scrolled text) ----- */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, 0, 0, 128, VISIBLE_TOP - 1);   /* mask background */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    u8g2_DrawStr(u8g2, 2, VISIBLE_TOP - 3, page->title);
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawHLine(u8g2, 0, VISIBLE_TOP - 1, 128);     /* separator */
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
        return true;
    }
    return false;
}
