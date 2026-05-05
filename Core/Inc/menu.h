#ifndef __MENU_H__
#define __MENU_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "ux_move.h"

#define MENU_LINE_HEIGHT  12
#define MENU_TITLE_HEIGHT  12

typedef struct menu_item {
    const char *name;
    void (*action)(void);
    const struct menu_page *submenu;
} menu_item_t;

typedef struct menu_page {
    const char *title;
    const menu_item_t *items;
    uint8_t count;
    const struct menu_page *parent;
} menu_page_t;

typedef enum {
    TRANS_NONE,
    TRANS_ENTER,
    TRANS_BACK,
} menu_trans_e;

typedef struct {
    const menu_page_t *current;
    uint8_t selected;
    anim_ctrl_t scroll_anim;
    int16_t scroll_target;
    anim_ctrl_t bar_anim;
    int16_t  bar_target_y;
    int16_t  bar_target_w;
    /* page transition */
    menu_trans_e       trans;
    const menu_page_t *trans_old;
    anim_ctrl_t        trans_anim;   /* value 0→100 drives positions */
} menu_state_t;

void menu_init(menu_state_t *state, const menu_page_t *root);
void menu_update(menu_state_t *state);
void menu_render(u8g2_t *u8g2, menu_state_t *state);

bool menu_key_up(menu_state_t *state);
bool menu_key_down(menu_state_t *state);
bool menu_key_enter(menu_state_t *state);
bool menu_key_back(menu_state_t *state);

#endif
