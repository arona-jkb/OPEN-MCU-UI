#ifndef __POPUP_CONFIRM_H__
#define __POPUP_CONFIRM_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "anim_engine.h"
#include "popup.h"

/* 确认回调 — 用户确认时调用 */
typedef void (*confirm_callback_t)(bool ok);

typedef struct {
    popup_state_e       state;
    const char         *text;             /* 提示文字             */
    confirm_callback_t  on_result;        /* 结果回调 (true=确认) */
    anim_ctrl_t         slide;            /* 滑入/滑出动画        */
    anim_ctrl_t         uline_x;          /* 下划线 X 移动动画    */
    int16_t             uline_x_targ;     /* 下划线 X 目标        */
    int16_t             uline_w_targ;     /* 下划线宽度目标       */
} popup_confirm_t;

void popup_confirm_init(popup_confirm_t *p, popup_base_t *b);
void popup_confirm_open(popup_confirm_t *p, const char *text,
                        confirm_callback_t on_result);

#endif
