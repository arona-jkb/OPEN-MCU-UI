#ifndef __UX_MOVE_H__
#define __UX_MOVE_H__

#include <stdint.h>
#include <stdbool.h>
#ifndef MAX_ANIM_NUM
#define MAX_ANIM_NUM  25   /* 过渡动画需要较多并发动画槽位 */
#endif

#include "main.h"

/* 动画状态机 */
typedef enum {
    ANIM_IDLE = 0,
    ANIM_PLAYING,
    ANIM_PAUSED,
    ANIM_FINISHED,
    ANIM_BACKING,
} anim_state_t;
// 动画步骤结构体(用于组成动画序列数组)
typedef struct {
    int16_t target_x;
    int16_t target_y;
    uint32_t duration_ms;
    void (*easing)(int32_t t, int32_t b, int32_t c, int32_t d, int32_t *out);
} anim_step_t;

/* 动画控制块 */
typedef struct {
    anim_state_t state;
    uint32_t start_time;      // 开始时的系统滴答（ms）
    uint32_t duration;        // 总时长（ms）
    int16_t start_x, start_y; // 起始坐标（像素，整数）
    int16_t end_x, end_y;     // 结束坐标
    int16_t cur_x, cur_y;     // 当前帧坐标
    void (*easing)(int32_t t, int32_t b, int32_t c, int32_t d, int32_t *out);
    int32_t elapsed_time;
    void (*on_finish)(void *element);// 动画结束回调
    //动画序列支持
    const anim_step_t *steps; // 指向动画序列数组的指针
    uint8_t step_count;          // 动画序列中的步骤数量
    uint8_t current_step;        // 当前动画步骤索引
    bool loop;                   // 是否循环播放动画序列
} anim_ctrl_t;

void anim_manager_update(void);

void quad_ease_out(int32_t t, int32_t b, int32_t c, int32_t d, int32_t *out);
void linear_ease(int32_t t, int32_t b, int32_t c, int32_t d, int32_t *out);

void anim_init(anim_ctrl_t *anim);
void anim_set_position(anim_ctrl_t *anim, int16_t x, int16_t y);
void anim_start_step(anim_ctrl_t *anim);
void anim_start(anim_ctrl_t *anim, int16_t sx, int16_t sy, int16_t ex, int16_t ey, uint32_t duration_ms,void *easing);
void anim_pause(anim_ctrl_t *anim);
void anim_resume(anim_ctrl_t *anim);
void anim_stop(anim_ctrl_t *anim);
void anim_back(anim_ctrl_t *anim);

bool anim_manager_is_idle(void);
static inline void anim_get_position(anim_ctrl_t *anim, int16_t *x, int16_t *y) // 获取当前动画坐标
{
    if (anim) { *x = anim->cur_x; *y = anim->cur_y; }
}
static inline bool anim_is_playing(anim_ctrl_t *anim) // 判断动画是否正在播放
{
    return anim && (anim->state == ANIM_PLAYING);
}



#endif