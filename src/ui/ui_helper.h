#ifndef UI_HELPER_H
#define UI_HELPER_H

#include <tic80.h>

#define TIC80_MAP_WIDTH (240)
#define TILE_W (TILE_SIZE)
#define TILE_H (TILE_SIZE)
#define FONT_H (TILE_SIZE)
#define COLOR_TRANSPARENT (0)
#define COLOR_TABLE_BG (7)
#define COLOR_BLACK (15)
#define COLOR_WHITE (12)

#define T_EMP_TOP (10)
#define T_EMP_MID (12)
#define T_EMP_BOT (14)
#define T_EMP_MID_ALT (28)
#define T_CAD_TOP (4)
#define T_CAD_TOP_ALT (2)
#define T_CAD_MID (6)
#define T_CAD_BOT (8)
#define T_CAD_TYPE (16)
#define T_CAD_VAL0 (32)
#define T_CAD_VAL1 (48)
#define T_CUR0 (256)
#define T_CUR1 (260)
#define T_CUR2 (264)
#define T_CUR3 (268)
#define T_ICON_UP (276)
#define T_ICON_DOWN (277)
#define T_ICON_UNDO (278)
#define CURSOR_ARROW (16)
#define CURSOR_POINTER (17)

extern uint8_t *transcolor0;

uint16_t u_get_tile_id(uint8_t x, uint8_t y);
uint16_t u_get_spr_id(uint8_t x, uint8_t y);
bool is_btn_pressed_once(int32_t index);
bool is_mouse_clicked(uint8_t *x, uint8_t *y);
int8_t get_mouse_scroll(void);

#endif
