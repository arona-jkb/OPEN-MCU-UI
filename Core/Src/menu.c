#include "menu.h"

static int16_t calc_scroll_target(const menu_state_t *state) {
    if (state->current->count <= 0) return 0;
    int16_t target = ((int16_t)state->selected - MENU_HIGHLIGHT_LINE) * MENU_LINE_HEIGHT;
    int16_t max_scroll = (int16_t)(state->current->count - 1 - MENU_HIGHLIGHT_LINE) * MENU_LINE_HEIGHT;
    if (max_scroll < 0) max_scroll = 0;
    if (target < 0) target = 0;
    if (target > max_scroll) target = max_scroll;
    return target;
}

static void start_scroll(menu_state_t *state) {
    int16_t target = calc_scroll_target(state);
    state->scroll_target = target;
    anim_start(&state->scroll_anim,
               0, state->scroll_anim.cur_y,
               0, target,
               250, quad_ease_out);
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

    /* title bar */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawHLine(u8g2, 0, MENU_TITLE_HEIGHT - 1, 128);
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    u8g2_DrawStr(u8g2, 2, MENU_TITLE_HEIGHT - 3, state->current->title);

    /* menu items */
    u8g2_SetFont(u8g2, u8g2_font_fub11_tf);
    for (uint8_t i = 0; i < state->current->count; i++) {
        int16_t y = MENU_TITLE_HEIGHT + (int16_t)i * MENU_LINE_HEIGHT - scroll + 10;
        if (y < MENU_TITLE_HEIGHT - 2 || y > 66) continue;

        if (i == state->selected) {
            u8g2_SetDrawColor(u8g2, 1);
            u8g2_DrawBox(u8g2, 0, y - 10, 128, 12);
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawStr(u8g2, 4, y, state->current->items[i].name);
            u8g2_SetDrawColor(u8g2, 1);
        } else {
            u8g2_DrawStr(u8g2, 4, y, state->current->items[i].name);
        }
    }
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
