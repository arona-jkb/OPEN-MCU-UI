#ifndef __SPLASH_H__
#define __SPLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include "u8g2.h"
#include "ux_move.h"

typedef enum {
    SPLASH_ENTER,
    SPLASH_HOLD,
    SPLASH_EXIT,
    SPLASH_DONE,
} splash_state_e;

typedef struct {
    splash_state_e state;
    anim_ctrl_t    line1;       /* "power by"      Y */
    anim_ctrl_t    line2;       /* "OPEN MCU UI"   Y */
    anim_ctrl_t    border;      /* white bottom border Y */
    uint32_t       hold_start;  /* HAL_GetTick() at hold begin */
} splash_t;

void splash_init(splash_t *s);
bool splash_done(const splash_t *s);
void splash_update(splash_t *s);
void splash_render(const splash_t *s, u8g2_t *u8g2);

#endif
