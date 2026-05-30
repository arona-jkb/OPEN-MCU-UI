#ifndef __MENU_H__
#define __MENU_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "ux_move.h"

/* 菜单项行高与标题栏高度(像素) */
#define MENU_LINE_HEIGHT  13
#define MENU_TITLE_HEIGHT  12
#define MENU_MAX_ITEMS      8              /* 单页最大菜单项数 (过渡动画槽位) */

/* ---------- 数据定义 ---------- */

/* 单个菜单项 */
typedef struct menu_item {
    const char *name;                     /* 显示文字                    */
    void (*action)(void);                 /* 确认键回调 (优先于 submenu) */
    const struct menu_page *submenu;      /* 子菜单页指针, 无则为 NULL  */
} menu_item_t;

/* 一页菜单 */
typedef struct menu_page {
    const char *title;                    /* 标题栏文字             */
    const menu_item_t *items;             /* 菜单项数组             */
    uint8_t count;                        /* 菜单项数量             */
    const struct menu_page *parent;       /* 父页面, 根页面为 NULL  */
} menu_page_t;

/* ---------- 页面切换动画 ---------- */

typedef enum {
    TRANS_NONE,                           /* 无切换            */
    TRANS_OLD_OUT,                        /* 旧页退出中        */
    TRANS_NEW_IN,                         /* 新页进入中        */
} menu_trans_e;

/* ---------- 运行时状态 ---------- */

typedef struct {
    const menu_page_t *current;           /* 当前显示的页面        */
    uint8_t selected;                     /* 当前选中项索引        */
    anim_ctrl_t scroll_anim;              /* 菜单文字滚动动画      */
    int16_t scroll_target;                /* 滚动目标 Y 偏移       */
    anim_ctrl_t bar_anim;                 /* 选择条位移动画        */
    int16_t  bar_target_y;                /* 选择条目标 Y 坐标     */
    int16_t  bar_target_w;                /* 选择条目标宽度        */
    /* 选中项文字水平滚动 (文字过长时) */
    anim_ctrl_t text_scroll_anim;         /* 文字水平滚动动画      */
    int16_t     text_scroll_target;       /* 文字滚动目标 X 偏移   */
    /* 右侧滚动进度条 */
    anim_ctrl_t prog_anim;                /* 进度条高度动画        */
    int16_t     prog_target;              /* 进度条目标高度        */
    /* 页面切换 — 每项独立动画 (同时结束, 距离越远速度越快) */
    menu_trans_e       trans;
    const menu_page_t *trans_old;
    uint8_t            trans_old_sel;     /* 旧页被选中项索引        */
    anim_ctrl_t        title_old;         /* 旧标题栏 Y 动画        */
    anim_ctrl_t        title_new;         /* 新标题栏 Y 动画        */
    anim_ctrl_t        items_old[MENU_MAX_ITEMS]; /* 旧页每项 Y    */
    anim_ctrl_t        items_new[MENU_MAX_ITEMS]; /* 新页每项 Y    */
} menu_state_t;

/* ---------- API ---------- */

void menu_init(menu_state_t *state, const menu_page_t *root);
void menu_update(menu_state_t *state);    /* 每帧调用, 处理切换收尾 */
void menu_render(u8g2_t *u8g2, menu_state_t *state);

bool menu_key_up(menu_state_t *state);
bool menu_key_down(menu_state_t *state);
bool menu_key_enter(menu_state_t *state);
bool menu_key_back(menu_state_t *state);

#endif
