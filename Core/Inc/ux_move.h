#ifndef __UX_MOVE_H__
#define __UX_MOVE_H__

#ifndef MAX_ANIM_NUM
#define MAX_ANIM_NUM  10   // 根据单片机 RAM 设置，一般 8~16 够用
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
    // 动画结束回调
    void (*on_finish)(void *element);
} anim_ctrl_t;

void anim_manager_update(void);

void quad_ease_out(int32_t t, int32_t b, int32_t c, int32_t d, int32_t *out);
void linear_ease(int32_t t, int32_t b, int32_t c, int32_t d, int32_t *out);


void anim_start(anim_ctrl_t *anim, 
                int16_t sx, int16_t sy, 
                int16_t ex, int16_t ey, 
                uint32_t duration_ms,void *easing);
void anim_pause(anim_ctrl_t *anim);
void anim_resume(anim_ctrl_t *anim);
void anim_stop(anim_ctrl_t *anim);
void anim_back(anim_ctrl_t *anim);





#endif