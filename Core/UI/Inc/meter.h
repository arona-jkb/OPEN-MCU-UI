#ifndef __METER_H__
#define __METER_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "ux_move.h"

#define METER_MAX_ITEMS  4               /* 一屏最多仪表项 (3~4) */

/* ---------- 数据定义 ---------- */

typedef struct {
    const char  *label;                 /* 标签文字 (如 "Temperature")    */
    int16_t     *value;                 /* 指向被展示的变量               */
    int16_t      min;                   /* 范围下限                        */
    int16_t      max;                   /* 范围上限                        */
    const char  *unit;                  /* 单位文字 (如 "°C", "%", "V")  */
    uint8_t      bar_h;                 /* 进度条高度 (0=仅文字)          */
} meter_item_t;

typedef struct {
    const char          *title;         /* 标题栏文字                     */
    const meter_item_t  *items;         /* 仪表项数组                     */
    uint8_t              count;         /* 项数                           */
} meter_page_t;

/*
 * 仪表页定义宏
 *
 *   METER_PAGE("Dashboard",
 *       { "Temperature", &g_temp,  0, 60, "C", 5 },
 *       { "Humidity",    &g_humi,  0, 100, "%", 5 },
 *   );
 */
#define METER_PAGE(pg_title, ...)                                          \
    { .title = (pg_title), .items = (const meter_item_t[]){ __VA_ARGS__ }, \
      .count = sizeof((const meter_item_t[]){ __VA_ARGS__ }) / sizeof(meter_item_t) }

/* ---------- 状态机 ---------- */

typedef enum {
    METER_IDLE,                           /* 空闲 (未激活)        */
    METER_ENTER,                          /* 入场动画中           */
    METER_ACTIVE,                         /* 可操作               */
    METER_EXIT,                           /* 退场动画中           */
} meter_trans_e;

/* ---------- 运行时状态 ---------- */

typedef struct {
    const meter_page_t *page;

    /* 全局入场/退场 */
    meter_trans_e trans;
    anim_ctrl_t   slide;                 /* 页面整体 Y 动画      */
    anim_ctrl_t   title_y;              /* 标题栏 Y              */

    /* 每行动画 */
    anim_ctrl_t item_x[METER_MAX_ITEMS]; /* 行水平飞入 X 动画    */
    anim_ctrl_t bar_fill_w[METER_MAX_ITEMS]; /* 进度条填充宽度动画 */
    int16_t     bar_fill_target[METER_MAX_ITEMS]; /* 填充宽度目标 */

    /* 数值格式化缓存 (仅用于退出时最后一次显示的正确值) */
    int16_t cached_value[METER_MAX_ITEMS];
} meter_state_t;

/* ---------- API ---------- */

void meter_init(meter_state_t *s);
void meter_open(meter_state_t *s, const meter_page_t *page);
void meter_close(meter_state_t *s);
bool meter_active(const meter_state_t *s);

void meter_update(meter_state_t *s);      /* 每帧调用, 推进状态机 */
void meter_render(const meter_state_t *s, u8g2_t *u8g2);

#endif
