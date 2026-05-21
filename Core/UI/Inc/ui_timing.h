#ifndef __UI_TIMING_H__
#define __UI_TIMING_H__

/* ================================================================
 *  全局动画时长参数 (ms)
 *
 *  所有 UI 模块 (menu / popup / splash) 统一引用此文件。
 *  调参只需改这里，不必散落到各 .c 中逐个翻找。
 * ================================================================ */

/* ---- 菜单 ---- */
#define BAR_ANIM_MS      200              /* 选择条移动/变形动画     */
#define SCROLL_ANIM_MS   350              /* 菜单文字边界滚动动画    */
#define TRANS_MS         800              /* 页面切换过渡动画        */

/* ---- 弹窗 ---- */
#define POPUP_OPEN_MS    300              /* 弹窗滑入时长            */
#define POPUP_CLOSE_MS   250              /* 弹窗滑出时长            */
#define TOAST_DURATION   1000             /* Toast 停留时长          */
#define TOAST_ANIM_MS    180              /* Toast 滑入/滑出时长     */

/* ---- 启动动画 ---- */
#define SPLASH_ENTER_MS  500              /* 文字飞入时长            */
#define SPLASH_HOLD_MS   1000             /* 静止展示时长            */
#define SPLASH_EXIT_MS   400              /* 文字飞出时长            */

#endif
