#ifndef __MENU_H__
#define __MENU_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "ux_move.h"

/* 菜单项行高与标题栏高度(像素) */
#define MENU_LINE_HEIGHT  11
#define MENU_TITLE_HEIGHT  12
#define MENU_MAX_ITEMS      8

/* ---------- 数据定义 ---------- */

typedef enum {
    MENU_TEXT,
    MENU_ICON,
} menu_style_e;

/* 标题栏对齐方式 */
typedef enum {
    TITLE_LEFT,                           /* 左对齐 (文字菜单)    */
    TITLE_CENTER,                         /* 居中   (图标菜单)    */
} menu_title_align_e;

typedef struct {
    const uint8_t *bitmap;
    uint8_t        w;
    uint8_t        h;
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
    menu_title_align_e title_align;       /* 标题栏对齐方式        */
    const menu_item_t *items;
    uint8_t            count;
    const struct menu_page *parent;
} menu_page_t;

/*
 * 菜单页定义宏
 *
 *   MENU_PAGE_TEXT  →  style=MENU_TEXT,  align=TITLE_LEFT
 *   MENU_PAGE_ICON  →  style=MENU_ICON,  align=TITLE_CENTER
 *
 *  如需覆盖对齐方式, 可手动写 menu_page_t 结构体。
 */
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

/* ---------- 页面切换动画 ---------- */

typedef enum {
    TRANS_NONE,
    TRANS_OLD_OUT,
    TRANS_NEW_IN,
} menu_trans_e;

/* ---------- 运行时状态 ---------- */

typedef struct {
    const menu_page_t *current;
    uint8_t selected;
    /* 文字菜单 */
    anim_ctrl_t scroll_anim;
    int16_t scroll_target;
    anim_ctrl_t bar_anim;
    int16_t  bar_target_y;
    int16_t  bar_target_w;
    anim_ctrl_t text_scroll_anim;
    int16_t     text_scroll_target;
    /* 图标菜单: 整行水平滚动 (选中项居中) */
    anim_ctrl_t icon_scroll_anim;
    int16_t     icon_scroll_target;
    /* 图标菜单: 入场/退场过渡 */
    anim_ctrl_t icon_trans_title_y;
    anim_ctrl_t icon_trans_label_y;
    anim_ctrl_t icon_trans_item_x[MENU_MAX_ITEMS];
    /* 图标菜单: 标签切换动画 */
    anim_ctrl_t icon_label_old_y;
    anim_ctrl_t icon_label_new_y;
    const char  *icon_label_old_name;
    const char  *icon_label_new_name;
    uint8_t      icon_label_phase;
    /* 进度条 */
    anim_ctrl_t prog_anim;
    int16_t     prog_target;
    /* 图标布局缓存 (避免每帧除法) */
    int16_t cached_slot_step;
    uint8_t cached_count;
    uint8_t cached_iw;
    /* 页面切换 */
    menu_trans_e       trans;
    const menu_page_t *trans_old;
    uint8_t            trans_old_sel;
    anim_ctrl_t        title_old;
    anim_ctrl_t        title_new;
    anim_ctrl_t        items_old[MENU_MAX_ITEMS];
    anim_ctrl_t        items_new[MENU_MAX_ITEMS];
} menu_state_t;

/* ---------- API ---------- */

void menu_init(menu_state_t *state, const menu_page_t *root);
void menu_update(menu_state_t *state);
void menu_render(u8g2_t *u8g2, menu_state_t *state);

bool menu_key_up(menu_state_t *state);
bool menu_key_down(menu_state_t *state);
bool menu_key_enter(menu_state_t *state);
bool menu_key_back(menu_state_t *state);

#endif
