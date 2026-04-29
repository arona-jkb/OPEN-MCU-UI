#include "ux_move.h"
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    anim_ctrl_t *anim_list[MAX_ANIM_NUM];  // 存储动画实例指针
    uint8_t count;                         // 当前已注册数量
} anim_manager_t;

static anim_manager_t g_anim_mgr = {0};    // 全局单例管理器

//*------------------------------------------核心缓动函数-------------------------------------------*/

/* 线性插值：t动画已经流逝的时间，b 动画坐标起始值，c 动画坐标总变化量，d 动画运行总时长，通过out输出当前值 */
 inline void linear_ease(int32_t t, int32_t b, int32_t c, int32_t d, int32_t *out) 
{
    *out = b + (c * t) / d;
}
/* 二次 ease-out 示例（可选） */
 void quad_ease_out(int32_t t, int32_t b, int32_t c, int32_t d, int32_t *out) 
{
 // 使用公式：f(t) = b + c × (2×t/d - t²/d²)
    
    if (d == 0) {
        *out = b + c;  // 防止除以零
        return;
    }
    
    // 使用16位精度（减少计算量，视觉差异不大）
    // 将时间归一化到[0, 256]范围，便于整数运算
    uint32_t normalized_t = (t << 8) / d;  // t/d 放大256倍（使用移位优化）
    
    // 计算 t²/d²，注意已经放大256倍，平方后是65536倍
    uint32_t t_squared = (normalized_t * normalized_t) >> 8;  // 除以256得到256倍
    
    // 计算 2×t/d - t²/d²，结果在[0, 256]范围内
    uint32_t progress = (2 * normalized_t) - t_squared;
    
    // 计算最终结果：b + c × progress / 256
    *out = b + ((int32_t)c * (int32_t)progress) / 256;
}
/*------------------------------------------内部动画管理器注册函数与移除函数-------------------------------------------*/

/* 添加一个动画实例到管理器（通常启动动画时自动注册） */
bool anim_register(anim_ctrl_t *anim) 
{
    if (g_anim_mgr.count >= MAX_ANIM_NUM) return false;
    for (int i = 0; i < g_anim_mgr.count; i++) 
    {
        if (g_anim_mgr.anim_list[i] == anim) return true; // 已存在
    }
    g_anim_mgr.anim_list[g_anim_mgr.count++] = anim;
    return true;
}

/* 从管理器中移除动画实例（动画结束或主动停止时调用） */
void anim_unregister(anim_ctrl_t *anim) 
{
    for (int i = 0; i < g_anim_mgr.count; i++) 
    {
        if (g_anim_mgr.anim_list[i] == anim) // 将最后一个元素移到当前位置，并减少计数
        {
            g_anim_mgr.anim_list[i] = g_anim_mgr.anim_list[--g_anim_mgr.count];
            return;
        }
    }
}

/*------------------------------------------外部公共函数-------------------------------------------*/
// 在 ux_move.c 中实现
void anim_init(anim_ctrl_t *anim) {
    if (!anim) return;
    anim->state = ANIM_IDLE;
    anim->start_time = 0;
    anim->duration = 0;
    anim->start_x = anim->start_y = 0;
    anim->end_x = anim->end_y = 0;
    anim->cur_x = anim->cur_y = 0;
    anim->easing = quad_ease_out;    // 默认线性
    anim->elapsed_time = 0;
    anim->on_finish = NULL;
}
void anim_set_position(anim_ctrl_t *anim, int16_t x, int16_t y) {
    if (!anim) return;
    anim->cur_x = x;
    anim->cur_y = y;
    anim->start_x = anim->end_x = x;
    anim->start_y = anim->end_y = y;
    anim->state = ANIM_IDLE;
}
/**
 * @brief 启动动画
 * 
 * 初始化动画控制块并启动动画播放。设置动画的起始状态、时间参数和运动轨迹，
 * 然后将动画实例注册到全局管理器中以便后续更新。
 * 
 * @param anim       指向动画控制块的指针
 * @param sx         起始X坐标（像素）
 * @param sy         起始Y坐标（像素）
 * @param ex         结束X坐标（像素）
 * @param ey         结束Y坐标（像素）
 * @param duration_ms 动画持续时间（毫秒）
 */
void anim_start(anim_ctrl_t *anim, 
                int16_t sx, int16_t sy, 
                int16_t ex, int16_t ey, 
                uint32_t duration_ms,void *easing) 
{
    if (!anim) return;
    anim->state = ANIM_PLAYING;
    anim->start_time = HAL_GetTick();
    anim->duration = duration_ms;
    anim->start_x = sx;
    anim->start_y = sy;
    anim->end_x   = ex;
    anim->end_y   = ey;
    anim->cur_x   = sx;
    anim->cur_y   = sy;
    anim->easing  = easing;
    
    anim_register(anim);           // 自动加入管理器
}
/**
 * @brief 暂停动画播放
 * 
 * 将正在播放的动画暂停，保存当前已播放的时间，以便后续恢复时能够
 * 从暂停的位置继续播放。只有处于播放状态的动画才能被暂停。
 * 无法暂停已反向播放的动画。
 * 
 * @param anim 指向要暂停的动画控制块的指针
 * 
 * @note 此函数会进行参数有效性检查，确保动画存在且处于播放状态
 * @note 暂停后动画状态变为ANIM_PAUSED，动画管理器会跳过更新但保留动画实例
 * @note 保存的已流逝时间用于anim_resume函数实现无缝恢复
 */

void anim_pause(anim_ctrl_t *anim) 
{
    if (anim && anim->state == ANIM_PLAYING) 
    {
        anim->state = ANIM_PAUSED;
        uint32_t now = HAL_GetTick();
        anim->elapsed_time = now - anim->start_time; // 保存已流逝时间
    }
}
/**
 * @brief 恢复暂停的动画
 * 
 * 将处于暂停状态的动画重新恢复播放，通过重新计算起始时间实现
 * 从暂停位置无缝继续播放。只有处于暂停状态的动画才能被恢复。
 * 无法恢复已反向播放的动画。
 * 
 * @param anim 指向要恢复的动画控制块的指针
 * 
 * @note 此函数会进行参数有效性检查，确保动画存在且处于暂停状态
 * @note 恢复后动画状态变为ANIM_PLAYING，动画管理器会重新开始更新
 * @note 通过重置起始时间实现无缝恢复，避免动画跳跃
 */

void anim_resume(anim_ctrl_t *anim) 
{
    if (anim && anim->state == ANIM_PAUSED) 
    {
        anim->state = ANIM_PLAYING;
        uint32_t now = HAL_GetTick();
        anim->start_time = now - anim->elapsed_time;// 重置起始时间，使得 now - start_time = elapsed_paused
    }
}
/**
 * @brief 停止动画播放
 * 
 * 强制停止动画播放，无论当前处于何种状态（播放、暂停、回溯等），
 * 都将动画状态重置为空闲并从动画管理器中移除。结束回调函数也将不会触发。
 * 
 * @param anim 指向要停止的动画控制块的指针
 * 
 * @note 此函数会进行参数有效性检查，确保动画指针有效
 * @note 停止后动画状态变为ANIM_IDLE，动画管理器会自动移除该动画
 * @note 与anim_pause不同，停止操作不可恢复，需要重新启动动画
 */

void anim_stop(anim_ctrl_t *anim) 
{
    if (!anim) return;
    anim->state = ANIM_IDLE;
    anim_unregister(anim);
}
/**
 * @brief 动画回溯播放
 * 
 * 将当前动画反向播放，从当前位置返回到起始位置。支持处理已完成动画的反向播放。但是结束回调不会继承。
 * 函数会先停止当前动画，然后启动一个新的反向动画，使用相同的已播放时间作为反向动画的持续时间。
 * 
 * @param anim 指向要回溯播放的动画控制块的指针
 * 
 * @note 此函数会进行参数有效性检查，确保动画指针有效
 * @note 仅对正在播放的动画或者已完成的动画（包括已经完成回溯的动画）有效，不能对已暂停或正在回溯的动画进行操作。
 * @note 如果是在动画的正向播放状态下，进行回溯播放。在回溯完成后再一次进行回溯，元素会回到正向播放被回溯打断的位置。
 * @note 对于已完成的动画，反向播放时间等于原动画总时长
 * @note 反向动画的起始位置是当前位置，结束位置是原动画的起始位置
 * @note 反向动画的持续时间等于原动画已播放的时间
 */

void anim_back(anim_ctrl_t *anim)
{   
    if (!anim||anim->state == ANIM_BACKING) return;
    uint32_t elapsed = HAL_GetTick() - anim->start_time;
    if (anim->state == ANIM_FINISHED)
    {
        elapsed = anim->duration;
    }
    anim_stop(anim);
    anim_start(anim, anim->cur_x, anim->cur_y, anim->start_x, anim->start_y, elapsed, anim->easing);
    anim->state = ANIM_BACKING;
}

/**
 * @brief 动画管理器更新函数
 * 
 * 主循环或者定时中断函数中定期调用的核心函数，负责更新所有注册动画的状态。
 * 遍历全局动画管理器中的所有动画实例，根据当前时间计算动画进度，
 * 更新动画坐标，处理动画结束逻辑，并自动清理已完成或异常状态的动画。
 * 
 * @note 此函数应在主循环中定期调用
 * @note 使用安全遍历算法，支持在循环过程中动态移除动画实例
 */
void anim_manager_update(void) 
{
    uint32_t now = HAL_GetTick();
    
    for (int i = 0; i < g_anim_mgr.count; ) 
    {
        anim_ctrl_t *anim = g_anim_mgr.anim_list[i];
        
        if (anim->state == ANIM_FINISHED||anim->state == ANIM_IDLE) 
        {
            anim_unregister(anim);// 如果是已完成状态或空闲状态，直接移除
            continue; // i 不变，因为当前元素已被替换
        }

        // 跳过暂停状态的动画，不更新但保留在管理器中
        if (anim->state == ANIM_PAUSED) 
        {
            i++; // 继续处理下一个动画
            continue;
        }

        uint32_t elapsed = now - anim->start_time;
        
        if (elapsed >= anim->duration) // 动画结束
        {
            anim->cur_x = anim->end_x;
            anim->cur_y = anim->end_y;
            anim->state = ANIM_FINISHED;
            if (anim->on_finish) // 如果有结束回调函数，调用它
            {
                anim->on_finish(anim);
            }
            anim_unregister(anim);  // 自动移除
        } 
        else // 动画进行中,更新动画坐标
        {
            int32_t dx = anim->end_x - anim->start_x;
            int32_t dy = anim->end_y - anim->start_y;
            int32_t cur_x, cur_y;
            anim->easing(elapsed, anim->start_x, dx, anim->duration, &cur_x);
            anim->easing(elapsed, anim->start_y, dy, anim->duration, &cur_y);
            anim->cur_x = cur_x;
            anim->cur_y = cur_y;
            i++;  // 只有未移除时才递增索引
        }
    }
}

