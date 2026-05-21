#ifndef __APP_UI_H__
#define __APP_UI_H__

#include "u8g2.h"
#include "menu.h"
#include <stdint.h>
#include <stdbool.h>

/* custom screen callback — render(id) dispatched by developer */
typedef void (*app_ui_render_fn)(u8g2_t *u8g2, uint8_t id);

void app_ui_init(u8g2_t *u8g2, const menu_page_t *root);
void app_ui_update(int8_t key);
void app_ui_render(u8g2_t *u8g2);

/* developer hooks */
void app_ui_set_custom_render(app_ui_render_fn render);
void app_ui_custom_screen_enter(uint8_t id);

/* popup short-cuts for action callbacks */
void app_ui_value_open(const char *title, int16_t *val,
                           int16_t min, int16_t max, int16_t step);
void app_ui_toggle_open(const char *title, bool *val,
                            const char *on, const char *off);
void app_ui_toast_show(const char *text);
void app_ui_goto_root(void);

#endif
