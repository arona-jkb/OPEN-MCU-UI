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

/* ========== numeric-adjust popup ========== */
typedef struct {
    const char *title;
    int16_t    *value;
    int16_t     min;
    int16_t     max;
    int16_t     step;
} popup_num_cfg_t;

typedef struct {
    popup_state_e  state;
    popup_num_cfg_t cfg;
    anim_ctrl_t    slide;
} popup_num_t;

void popup_num_init(popup_num_t *p);
void popup_num_open(popup_num_t *p, const char *title, int16_t *value,
                    int16_t min, int16_t max, int16_t step);
bool popup_num_active(const popup_num_t *p);
void popup_num_update(popup_num_t *p, int8_t key);
void popup_num_render(const popup_num_t *p, u8g2_t *u8g2);

/* ========== boolean-toggle popup ========== */
typedef struct {
    const char *title;
    bool       *value;
    const char *text_on;
    const char *text_off;
} popup_bool_cfg_t;

typedef struct {
    popup_state_e    state;
    popup_bool_cfg_t cfg;
    anim_ctrl_t      slide;
} popup_bool_t;

void popup_bool_init(popup_bool_t *p);
void popup_bool_open(popup_bool_t *p, const char *title, bool *value,
                     const char *text_on, const char *text_off);
bool popup_bool_active(const popup_bool_t *p);
void popup_bool_update(popup_bool_t *p, int8_t key);
void popup_bool_render(const popup_bool_t *p, u8g2_t *u8g2);

/* ========== toast notification popup ========== */
typedef struct {
    popup_state_e state;
    const char   *text;
    uint32_t      open_time;
    anim_ctrl_t   slide;
} popup_toast_t;

void popup_toast_init(popup_toast_t *p);
void popup_toast_show(popup_toast_t *p, const char *text);
bool popup_toast_active(const popup_toast_t *p);
void popup_toast_update(popup_toast_t *p);
void popup_toast_render(const popup_toast_t *p, u8g2_t *u8g2);

#endif
