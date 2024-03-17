#include <tic80.h>
#include "ui_helper.h"

static uint8_t transcolor0obj = {0};
uint8_t *transcolor0 = &transcolor0obj;
static uint8_t btn_status[4];
static bool mouse_status = false;

void u_set_map_at(uint8_t x, uint8_t y, uint8_t tile) { mset(x, y, tile); }

uint16_t u_get_tile_id(uint8_t x, uint8_t y) { return y * 16 + x; }

uint16_t u_get_spr_id(uint8_t x, uint8_t y) { return y * 16 + x + 256; }

bool is_btn_pressed_once(int32_t index) {
    uint8_t player = (uint8_t)((index / 8) % 4);
    uint8_t key = (uint8_t)(index % 8);
    uint8_t status = (GAMEPADS[player] & (1 << key)) >> key;
    uint8_t old_status = (btn_status[player] & (1 << key)) >> key;
    bool pressed = false;
    if (status != old_status) {
        if (status) {
            pressed = true;
        }
        if (status) {
            // set 1
            btn_status[player] |= (1 << key);
        } else {
            // set 0
            btn_status[player] &= (~(1 << key));
        }
    }
    return pressed;
}

bool is_mouse_clicked(uint8_t *x, uint8_t *y) {
    bool btn_left = ((bool) MOUSE->left);
    uint8_t cx = (MOUSE->x) - 8;
    uint8_t cy = (MOUSE->y) - 4;
    if (mouse_status) {
        if (!btn_left) {
            mouse_status = false;
        }
    } else {
        if (btn_left) {
            mouse_status = true;
            // click down
            if (cx < WIDTH && cy < HEIGHT) {
                // clicked inside the screen
                *x = cx;
                *y = cy;
                return true;
            }
        }
    }
    return false;
}

int8_t get_mouse_scroll(void) {
    return (int8_t) (MOUSE->v);
}
