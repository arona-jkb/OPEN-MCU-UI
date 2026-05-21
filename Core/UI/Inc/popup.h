#ifndef __POPUP_H__
#define __POPUP_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "ux_move.h"

/* ========== shared state ========== */
typedef enum {
    POPUP_IDLE,
    POPUP_OPENING,
    POPUP_ACTIVE,
    POPUP_CLOSING,
} popup_state_e;

/* ========== popup manager ========== */
typedef bool (*popup_active_fn)(void *p);
typedef void (*popup_update_fn)(void *p, int8_t key);
typedef void (*popup_render_fn)(void *p, u8g2_t *u8g2);

typedef struct {
    void            *instance;
    popup_active_fn  active;
    popup_update_fn  update;
    popup_render_fn  render;
} popup_base_t;

#define MAX_POPUP_NUM  8

void popup_mgr_init(void);
bool popup_mgr_register(popup_base_t *p);
bool popup_mgr_any_active(void);
void popup_mgr_update(int8_t key);
void popup_mgr_render(u8g2_t *u8g2);

/* ========== numeric-adjust popup ========== */
typedef struct {
    const char *title;
    int16_t    *value;
    int16_t     min;
    int16_t     max;
    int16_t     step;
} popup_value_cfg_t;

typedef struct {
    popup_state_e  state;
    popup_value_cfg_t cfg;
    anim_ctrl_t    slide;
} popup_value_t;

void popup_value_init(popup_value_t *p, popup_base_t *b);
void popup_value_open(popup_value_t *p, const char *title, int16_t *value,
                    int16_t min, int16_t max, int16_t step);

/* ========== boolean-toggle popup ========== */
typedef struct {
    const char *title;
    bool       *value;
    const char *text_on;
    const char *text_off;
} popup_toggle_cfg_t;

typedef struct {
    popup_state_e    state;
    popup_toggle_cfg_t cfg;
    anim_ctrl_t      slide;
} popup_toggle_t;

void popup_toggle_init(popup_toggle_t *p, popup_base_t *b);
void popup_toggle_open(popup_toggle_t *p, const char *title, bool *value,
                     const char *text_on, const char *text_off);

/* ========== toast notification popup ========== */
typedef struct {
    popup_state_e state;
    const char   *text;
    uint32_t      open_time;
    anim_ctrl_t   slide;
} popup_toast_t;

void popup_toast_init(popup_toast_t *p, popup_base_t *b);
void popup_toast_show(popup_toast_t *p, const char *text);

#endif
