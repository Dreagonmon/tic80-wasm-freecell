#include "ui_helper.h"
#include "ui_layers.h"
#include "ui_stack.h"
#include <stddef.h>

#define REDRAW_TICKS (60)
#define MENU_COUNT (4)
#define MENU_OFFSET_TILE_X (12)
#define MENU_OFFSET_TILE_Y (13)

static int8_t sel_idx = 0;

static void draw_title_screen(void) {
    // bg
    FRAMEBUFFER->BORDER_COLOR_AND_OVR_TRANSPARENCY = COLOR_TRANSPARENT;
    map(0, 0, WIDTH_TILES, HEIGHT_TILES, 0, 0, NULL, 0, 1, TIC80_PARAM_IGNORE);
    // menu
    uint8_t start_x = MENU_OFFSET_TILE_X * TILE_W;
    uint8_t start_y = MENU_OFFSET_TILE_Y * TILE_H;
    print("New Game", start_x, start_y, COLOR_WHITE, false, 1, false);
    start_y += TILE_H;
    print("Select Game", start_x, start_y, COLOR_WHITE, false, 1, false);
    start_y += TILE_H;
    print("Load Game", start_x, start_y, COLOR_WHITE, false, 1, false);
    start_y += TILE_H;
    print("Exit", start_x, start_y, COLOR_WHITE, false, 1, false);
    // select
    start_x = MENU_OFFSET_TILE_X * TILE_W - 2 - TILE_W;
    start_y = (MENU_OFFSET_TILE_Y + (uint8_t)sel_idx) * TILE_H - 2;
    spr(T_CUR0 + 3, start_x, start_y, transcolor0, 1, 1, false, false,
        1, 1);
}

static void onFocus(void) { draw_title_screen(); }

static void tic(void) {
    bool need_redraw = false;
    if (btnp(BUTTON_CODE_P1_UP, TIC80_PARAM_IGNORE, TIC80_PARAM_IGNORE)) {
        sel_idx += MENU_COUNT - 1;
        sel_idx %= MENU_COUNT;
        need_redraw = true;
    }
    if (btnp(BUTTON_CODE_P1_DOWN, TIC80_PARAM_IGNORE, TIC80_PARAM_IGNORE)) {
        sel_idx += 1;
        sel_idx %= MENU_COUNT;
        need_redraw = true;
    }
    if (btnp(BUTTON_CODE_P1_A, TIC80_PARAM_IGNORE, TIC80_PARAM_IGNORE)) {
        switch (sel_idx) {
        case 0:
            // new game
            ui_push_layer(layer_freecell_game);
            return;
        case 3:
            // exit
            ui_pop_layer();
            return;
        }
    }
    if (need_redraw) {
        draw_title_screen();
    }
}

static UILayerObject layerObject = {.onFocus = onFocus, .tic = tic};

UILayer layer_title_screen = &layerObject;
