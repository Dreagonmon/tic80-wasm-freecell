#include "ui_helper.h"
#include "ui_layers.h"
#include "ui_stack.h"
#include <stddef.h>

#define REDRAW_TICKS (60)
#define MENU_COUNT (2)
#define MENU_OFFSET_TILE_X (12)
#define MENU_OFFSET_TILE_Y (13)

const char TEXT_START_GAME[] = "Start Game";
const char TEXT_EXIT[] = "Exit";
static int8_t sel_idx = 0;
static bool layer_need_redraw = true;

static void draw_title_screen(void) {
    // bg
    FRAMEBUFFER->BORDER_COLOR_AND_OVR_TRANSPARENCY = COLOR_TRANSPARENT;
    map(0, 0, WIDTH_TILES, HEIGHT_TILES, 0, 0, NULL, 0, 1, TIC80_PARAM_IGNORE);
    // menu
    uint8_t start_x = MENU_OFFSET_TILE_X * TILE_W;
    uint8_t start_y = MENU_OFFSET_TILE_Y * TILE_H;
    print(TEXT_START_GAME, start_x, start_y, COLOR_WHITE, false, 1, false);
    start_y += TILE_H;
    // print("Select Game", start_x, start_y, COLOR_WHITE, false, 1, false);
    // start_y += TILE_H;
    print(TEXT_EXIT, start_x, start_y, COLOR_WHITE, false, 1, false);
    // select
    start_x = MENU_OFFSET_TILE_X * TILE_W - 2 - TILE_W;
    start_y = (MENU_OFFSET_TILE_Y + (uint8_t)sel_idx) * TILE_H - 2;
    spr(T_CUR0 + 3, start_x, start_y, transcolor0, 1, 1, false, false, 1, 1);
}

static uint8_t get_mouse_on_element(uint8_t mx, uint8_t my) {
    uint8_t start_x = MENU_OFFSET_TILE_X * TILE_W;
    uint8_t start_y = MENU_OFFSET_TILE_Y * TILE_H;
    uint8_t text_width =
        print(TEXT_START_GAME, start_x, -FONT_H, COLOR_WHITE, false, 1, false);
    if (mx >= start_x && mx < (start_x + text_width) && my >= start_y &&
        my < (start_y + FONT_H)) {
        return 1;
    }
    start_y += TILE_H;
    text_width =
        print(TEXT_EXIT, start_x, -FONT_H, COLOR_WHITE, false, 1, false);
    if (mx >= start_x && mx < (start_x + text_width) && my >= start_y &&
        my < (start_y + FONT_H)) {
        return 2;
    }
    return 0;
}

static uint8_t process_mouse_event(void) {
    uint8_t mx, my, elem_id;
    bool clicked = is_mouse_clicked(&mx, &my);
    elem_id = get_mouse_on_element(mx, my);
    if (mx != 0 || my != 0) { // for retroarch emu mouse, hide at 0,0
        if (elem_id) {
            FRAMEBUFFER->MOUSE_CURSOR = CURSOR_POINTER;
        } else {
            FRAMEBUFFER->MOUSE_CURSOR = CURSOR_ARROW;
        }
    } else {
        FRAMEBUFFER->MOUSE_CURSOR = 0;
    }
    if (clicked) {
        return elem_id;
    }
    return 0;
}

static void do_start_game(void) { ui_push_layer(layer_random_freecell_game); }

static void do_exit(void) { ui_pop_layer(); }

static void on_focus(void) { layer_need_redraw = true; }

static void tic(void) {
    bool need_redraw = layer_need_redraw;
    // button
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
            do_start_game();
            return;
        case (MENU_COUNT - 1):
            // exit
            do_exit();
            return;
        }
    }
    // mouse
    uint8_t mouse_evt = process_mouse_event();
    switch (mouse_evt) {
    case 1:
        // new game
        do_start_game();
        return;
    case 2:
        // exit
        do_exit();
        return;
    }
    // draw
    if (need_redraw) {
        draw_title_screen();
        layer_need_redraw = false;
    }
}

static UILayerObject layer_object = {.on_focus = on_focus, .tic = tic};

UILayer layer_title_screen = &layer_object;
