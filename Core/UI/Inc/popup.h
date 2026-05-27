#ifndef __POPUP_H__
#define __POPUP_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "ux_move.h"

/* ---------- 共享状态 ---------- */

typedef enum {
    POPUP_IDLE,                           /* 空闲      */
    POPUP_OPENING,                        /* 打开动画中 */
    POPUP_ACTIVE,                         /* 可操作    */
    POPUP_CLOSING,                        /* 关闭动画中 */
} popup_state_e;

/* ---------- 弹窗管理器 ---------- */

typedef bool (*popup_active_fn)(void *p);
typedef void (*popup_update_fn)(void *p, int8_t key);
typedef void (*popup_render_fn)(void *p, u8g2_t *u8g2);

/* 管理器基类 — 每个弹窗实例配套一个, init 时自动注册 */
typedef struct {
    void            *instance;
    popup_active_fn  active;
    popup_update_fn  update;
    popup_render_fn  render;
} popup_base_t;

#define MAX_POPUP_NUM  8

void popup_mgr_init(void);
bool popup_mgr_register(popup_base_t *p);
bool popup_mgr_any_active(void);          /* 任一弹窗激活中 */
void popup_mgr_update(int8_t key);        /* 遍历所有弹窗的 update */
void popup_mgr_render(u8g2_t *u8g2);      /* 遍历所有弹窗的 render */

/* ---------- 数值调节弹窗 ---------- */

typedef struct {
    const char *title;
    int16_t    *value;                    /* 指向被调节的变量 */
    int16_t     min;
    int16_t     max;
    int16_t     step;
} popup_value_cfg_t;

typedef struct {
    popup_state_e     state;
    popup_value_cfg_t cfg;
    anim_ctrl_t       slide;              /* 滑入/滑出动画 */
    anim_ctrl_t       bar_anim;           /* 进度条填充宽度动画 */
    int16_t           bar_target;         /* 进度条目标填充宽度 */
} popup_value_t;

void popup_value_init(popup_value_t *p, popup_base_t *b);
void popup_value_open(popup_value_t *p, const char *title, int16_t *value,
                      int16_t min, int16_t max, int16_t step);

/* ---------- 开关弹窗 ---------- */

typedef struct {
    const char *title;
    bool       *value;                    /* 指向被翻转的变量 */
    const char *text_on;
    const char *text_off;
} popup_toggle_cfg_t;

typedef struct {
    popup_state_e      state;
    popup_toggle_cfg_t cfg;
    anim_ctrl_t        slide;
    anim_ctrl_t        knob_x;            /* 拨杆 X 坐标动画 */
    int16_t            knob_target;       /* 拨杆目标 X */
} popup_toggle_t;

void popup_toggle_init(popup_toggle_t *p, popup_base_t *b);
void popup_toggle_open(popup_toggle_t *p, const char *title, bool *value,
                       const char *text_on, const char *text_off);

/* ---------- Toast 通知弹窗 ---------- */

typedef struct {
    popup_state_e state;
    const char   *text;                   /* 显示文字(指针, 不拷贝) */
    uint32_t      open_time;              /* 打开时刻 (HAL_GetTick) */
    anim_ctrl_t   slide;
} popup_toast_t;

void popup_toast_init(popup_toast_t *p, popup_base_t *b);
void popup_toast_show(popup_toast_t *p, const char *text);

#endif
