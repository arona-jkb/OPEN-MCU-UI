#ifndef __SPLASH_H__
#define __SPLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "ux_move.h"

/* 启动动画状态机 */
typedef enum {
    SPLASH_ENTER,                         /* 文字飞入    */
    SPLASH_HOLD,                          /* 静止展示    */
    SPLASH_EXIT,                          /* 文字飞出    */
    SPLASH_DONE,                          /* 动画结束    */
} splash_state_e;

typedef struct {
    splash_state_e state;
    anim_ctrl_t    line1;                 /* "power by"       动画 Y */
    anim_ctrl_t    line2;                 /* "OPEN MCU UI"    动画 Y */
    anim_ctrl_t    border;                /* 底部白色分界线    动画 Y */
    uint32_t       hold_start;            /* 静止阶段起始时刻 (tick) */
} splash_t;

void splash_init(splash_t *s);
bool splash_done(const splash_t *s);
void splash_update(splash_t *s);          /* 每帧调用, 推进状态机 */
void splash_render(const splash_t *s, u8g2_t *u8g2);

/* 一体化渲染: ClearBuffer → 背景(退出阶段) → 前景 → SendBuffer */
void splash_render_frame(const splash_t *s, u8g2_t *u8g2,
                         void (*bg_render)(void *ctx, u8g2_t *u8g2),
                         void *bg_ctx);

#endif
