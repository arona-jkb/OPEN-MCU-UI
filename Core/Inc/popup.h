#ifndef __POPUP_H__
#define __POPUP_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "ux_move.h"

typedef struct {
    const char *title;
    int16_t    *value;
    int16_t     min;
    int16_t     max;
    int16_t     step;
} popup_config_t;

typedef enum {
    POPUP_IDLE,
    POPUP_OPENING,
    POPUP_ACTIVE,
    POPUP_CLOSING,
} popup_state_e;

typedef struct {
    popup_state_e  state;
    popup_config_t cfg;
    anim_ctrl_t    slide_anim;
} popup_t;

void popup_init(popup_t *p);
void popup_open(popup_t *p, const char *title, int16_t *value,
                int16_t min, int16_t max, int16_t step);
bool popup_is_active(const popup_t *p);
void popup_update(popup_t *p, int8_t key);
void popup_render(const popup_t *p, u8g2_t *u8g2);

#endif
