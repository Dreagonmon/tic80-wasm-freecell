#include <tic80.h>

static uint8_t transcolor0obj = {0};
uint8_t *transcolor0 = &transcolor0obj;
static uint8_t btn_status[4];

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
