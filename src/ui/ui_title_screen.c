#include "ui_helper.h"
#include "ui_layers.h"
#include "ui_stack.h"
#include <stddef.h>

#define REDRAW_TICKS (60)
#define MENU_COUNT (4)
#define MENU_OFFSET_TILE_X (12)
#define MENU_OFFSET_TILE_Y (13)

static int8_t sel_idx = 0;
static bool layer_need_redraw = true;

static void draw_title_screen(void) {
    // bg
    FRAMEBUFFER->BORDER_COLOR_AND_OVR_TRANSPARENCY = COLOR_TRANSPARENT;
    map(0, 0, WIDTH_TILES, HEIGHT_TILES, 0, 0, NULL, 0, 1, TIC80_PARAM_IGNORE);
    // menu
    uint8_t start_x = MENU_OFFSET_TILE_X * TILE_W;
    uint8_t start_y = MENU_OFFSET_TILE_Y * TILE_H;
    print("Random Game", start_x, start_y, COLOR_WHITE, false, 1, false);
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

static void on_focus(void) { layer_need_redraw = true; }

static void tic(void) {
    bool need_redraw = layer_need_redraw;
    if (is_btn_pressed_once(BUTTON_CODE_P1_UP)) {
        sel_idx += MENU_COUNT - 1;
        sel_idx %= MENU_COUNT;
        need_redraw = true;
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_DOWN)) {
        sel_idx += 1;
        sel_idx %= MENU_COUNT;
        need_redraw = true;
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_A)) {
        switch (sel_idx) {
        case 0:
            // new game
            ui_push_layer(layer_random_freecell_game);
            return;
        case 3:
            // exit
            ui_pop_layer();
            return;
        }
    }
    if (need_redraw) {
        draw_title_screen();
        layer_need_redraw = false;
    }
}

static UILayerObject layer_object = {.on_focus = on_focus, .tic = tic};

UILayer layer_title_screen = &layer_object;
