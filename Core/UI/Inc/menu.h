#ifndef __MENU_H__
#define __MENU_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "anim_engine.h"

#define MENU_LINE_HEIGHT  11
#define MENU_TITLE_HEIGHT  12
#define MENU_MAX_ITEMS      8

/* ---------- 数据定义 ---------- */

typedef enum { MENU_TEXT, MENU_ICON } menu_style_e;
typedef enum { TITLE_LEFT, TITLE_CENTER } menu_title_align_e;

typedef struct {
    const uint8_t *bitmap;
    uint8_t        w, h;
} menu_icon_t;

typedef struct menu_item {
    const char *name;
    menu_icon_t icon;
    void (*action)(void);
    const struct menu_page *submenu;
} menu_item_t;

typedef struct menu_page {
    const char        *title;
    menu_style_e       style;
    menu_title_align_e title_align;
    const menu_item_t *items;
    uint8_t            count;
    const struct menu_page *parent;
} menu_page_t;

#define MENU_PAGE_TEXT(pg_title, parent_ptr, ...)                              \
    { .title = (pg_title), .style = MENU_TEXT, .title_align = TITLE_LEFT,      \
      .parent = (parent_ptr),                                                   \
      .items = (const menu_item_t[]){ __VA_ARGS__ },                        \
      .count = sizeof((const menu_item_t[]){ __VA_ARGS__ }) / sizeof(menu_item_t) }

#define MENU_PAGE_ICON(pg_title, parent_ptr, ...)                              \
    { .title = (pg_title), .style = MENU_ICON, .title_align = TITLE_CENTER,    \
      .parent = (parent_ptr),                                                   \
      .items = (const menu_item_t[]){ __VA_ARGS__ },                        \
      .count = sizeof((const menu_item_t[]){ __VA_ARGS__ }) / sizeof(menu_item_t) }

/* ---------- 画面切换回调 ---------- */

typedef void (*menu_trans_done_fn)(void *ctx);

typedef enum { TRANS_NONE, TRANS_OLD_OUT, TRANS_NEW_IN } menu_trans_e;

typedef struct {
    const menu_page_t *current;
    uint8_t selected;

    /* 文字菜单 */
    anim_ctrl_t scroll_anim;
    int16_t scroll_target;
    anim_ctrl_t bar_anim;
    int16_t  bar_target_y, bar_target_w;
    anim_ctrl_t text_scroll_anim;
    int16_t     text_scroll_target;

    /* 图标菜单 */
    anim_ctrl_t icon_scroll_anim;
    int16_t     icon_scroll_target;
    anim_ctrl_t icon_trans_title_y;
    anim_ctrl_t icon_trans_label_y;
    anim_ctrl_t icon_trans_item_x[MENU_MAX_ITEMS];
    anim_ctrl_t icon_label_old_y;
    anim_ctrl_t icon_label_new_y;
    const char  *icon_label_old_name, *icon_label_new_name;
    uint8_t      icon_label_phase;

    /* 进度条 */
    anim_ctrl_t prog_anim;
    int16_t     prog_target;

    /* 布局缓存 */
    int16_t cached_slot_step;
    uint8_t cached_count, cached_iw;

    /* ---- 画面过渡 ---- */
    menu_trans_e       trans;
    menu_trans_done_fn trans_cb;          /* OLD_OUT 完成后回调 (NULL=默认子菜单) */
    void              *trans_cb_ctx;
    const menu_page_t *trans_old;
    uint8_t            trans_old_sel;
    anim_ctrl_t        title_old, title_new;
    anim_ctrl_t        items_old[MENU_MAX_ITEMS], items_new[MENU_MAX_ITEMS];
} menu_state_t;

/* ---------- API ---------- */

void menu_init(menu_state_t *state, const menu_page_t *root);
void menu_update(menu_state_t *state);
void menu_render(u8g2_t *u8g2, menu_state_t *state);

/* 画面过渡: 启动离开动画 (从当前页退出), done 时调用 trans_cb */
void menu_trans_out(menu_state_t *state, menu_trans_done_fn cb, void *ctx);

/* 画面过渡: 启动进入动画 (回到 current 页) */
void menu_trans_in(menu_state_t *state);

/* 按键 (text/icon 通用) */
bool menu_key_up(menu_state_t *state);
bool menu_key_down(menu_state_t *state);
bool menu_key_enter(menu_state_t *state);
bool menu_key_back(menu_state_t *state);

#endif
