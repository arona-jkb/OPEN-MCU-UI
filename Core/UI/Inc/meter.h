#ifndef __METER_H__
#define __METER_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "anim_engine.h"

/* ================================================================
 *  meter_bar — 进度条型仪表盘 (标题栏 + 多项数值进度条)
 * ================================================================ */

#define METER_BAR_MAX_ITEMS  4               /* 一屏最多仪表项 (3~4) */

/* ---------- 数据定义 ---------- */

typedef struct {
    const char  *label;                 /* 标签文字 (如 "Temperature")    */
    int16_t     *value;                 /* 指向被展示的变量               */
    int16_t      min;                   /* 范围下限                        */
    int16_t      max;                   /* 范围上限                        */
    const char  *unit;                  /* 单位文字 (如 "°C", "%", "V")  */
    uint8_t      bar_h;                 /* 进度条高度 (0=仅文字)          */
} meter_bar_item_t;

typedef struct {
    const char            *title;       /* 标题栏文字                     */
    const meter_bar_item_t *items;      /* 仪表项数组                     */
    uint8_t                count;       /* 项数                           */
} meter_bar_page_t;

/*
 * 仪表页定义宏
 *
 *   METER_BAR_PAGE("Dashboard",
 *       { "Temperature", &g_temp,  0, 60, "C", 5 },
 *       { "Humidity",    &g_humi,  0, 100, "%", 5 },
 *   );
 */
#define METER_BAR_PAGE(pg_title, ...)                                          \
    { .title = (pg_title), .items = (const meter_bar_item_t[]){ __VA_ARGS__ }, \
      .count = sizeof((const meter_bar_item_t[]){ __VA_ARGS__ }) / sizeof(meter_bar_item_t) }

/* ---------- 状态机 ---------- */

typedef enum {
    METER_BAR_IDLE,                       /* 空闲 (未激活)        */
    METER_BAR_ENTER,                      /* 入场动画中           */
    METER_BAR_ACTIVE,                     /* 可操作               */
    METER_BAR_EXIT,                       /* 退场动画中           */
} meter_bar_trans_e;

/* ---------- 运行时状态 ---------- */

typedef struct {
    const meter_bar_page_t *page;

    /* 全局入场/退场 */
    meter_bar_trans_e trans;
    anim_ctrl_t   slide;                 /* 页面整体 Y 动画      */
    anim_ctrl_t   title_y;              /* 标题栏 Y              */

    /* 每行动画 */
    anim_ctrl_t item_x[METER_BAR_MAX_ITEMS]; /* 行水平飞入 X 动画    */
    anim_ctrl_t bar_fill_w[METER_BAR_MAX_ITEMS]; /* 进度条填充宽度动画 */
    int16_t     bar_fill_target[METER_BAR_MAX_ITEMS]; /* 填充宽度目标 */

    /* 数值格式化缓存 (仅用于退出时最后一次显示的正确值) */
    int16_t cached_value[METER_BAR_MAX_ITEMS];
} meter_bar_state_t;

/* ---------- API ---------- */

void meter_bar_init(meter_bar_state_t *s);
void meter_bar_open(meter_bar_state_t *s, const meter_bar_page_t *page);
void meter_bar_close(meter_bar_state_t *s);
bool meter_bar_active(const meter_bar_state_t *s);

void meter_bar_update(meter_bar_state_t *s);      /* 每帧调用, 推进状态机 */
void meter_bar_render(const meter_bar_state_t *s, u8g2_t *u8g2);


/* ================================================================
 *  meter_quad — 四象限型仪表盘 (纯参数数值展示, 无进度条)
 *
 *  布局: 2x2 四象限网格
 *    参数名称在上方居中 (小字)
 *    数值居中突出显示 (大字粗体)
 *    单位以角标形式跟在数值后面 (小字下标)
 * ================================================================ */

#define METER_QUAD_MAX_ITEMS  4            /* 固定 4 象限 */

/* ---------- 数据定义 ---------- */

typedef struct {
    const char  *label;                 /* 参数名称 (如 "Temperature")    */
    int16_t     *value;                 /* 指向被展示的变量               */
    const char  *unit;                  /* 单位 (如 "°C", "%", "mV")    */
} meter_quad_item_t;

typedef struct {
    const char              *title;     /* 标题栏文字                     */
    const meter_quad_item_t *items;     /* 象限项数组 (4 项)              */
    uint8_t                  count;     /* 项数 (通常 = 4)                */
} meter_quad_page_t;

/*
 * 四象限仪表页定义宏
 *
 *   METER_QUAD_PAGE("Monitor",
 *       { "Temp",  &g_temp,  "°C"  },
 *       { "Humi",  &g_humi,  "%"   },
 *       { "Volt",  &g_volt,  "mV"  },
 *       { "Press", &g_press, "hPa" },
 *   );
 */
#define METER_QUAD_PAGE(pg_title, ...)                                            \
    { .title = (pg_title), .items = (const meter_quad_item_t[]){ __VA_ARGS__ },   \
      .count = sizeof((const meter_quad_item_t[]){ __VA_ARGS__ }) / sizeof(meter_quad_item_t) }

/* ---------- 状态机 ---------- */

typedef enum {
    METER_QUAD_IDLE,                      /* 空闲 (未激活)        */
    METER_QUAD_ENTER,                     /* 入场动画中           */
    METER_QUAD_ACTIVE,                    /* 可操作               */
    METER_QUAD_EXIT,                      /* 退场动画中           */
} meter_quad_trans_e;

/* ---------- 运行时状态 ---------- */

typedef struct {
    const meter_quad_page_t *page;

    meter_quad_trans_e trans;
    anim_ctrl_t title_y;                          /* 标题栏 Y 动画        */
    anim_ctrl_t item_x[METER_QUAD_MAX_ITEMS];     /* 每象限 X 方向动画    */
    anim_ctrl_t item_y[METER_QUAD_MAX_ITEMS];     /* 每象限 Y 方向动画    */
} meter_quad_state_t;

/* ---------- API ---------- */

void meter_quad_init(meter_quad_state_t *s);
void meter_quad_open(meter_quad_state_t *s, const meter_quad_page_t *page);
void meter_quad_close(meter_quad_state_t *s);
bool meter_quad_active(const meter_quad_state_t *s);

void meter_quad_update(meter_quad_state_t *s);      /* 每帧调用, 推进状态机 */
void meter_quad_render(const meter_quad_state_t *s, u8g2_t *u8g2);

#endif
