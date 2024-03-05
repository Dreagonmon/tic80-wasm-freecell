#include <tic80.h>

static uint8_t transcolor0obj = {0};
uint8_t *transcolor0 = &transcolor0obj;

void u_set_map_at(uint8_t x, uint8_t y, uint8_t tile) { mset(x, y, tile); }

uint16_t u_get_tile_id(uint8_t x, uint8_t y) { return y * 16 + x; }

uint16_t u_get_spr_id(uint8_t x, uint8_t y) { return y * 16 + x + 256; }
