#include "ui_helper.h"
#include "ui_layers.h"
#include "ui_stack.h"
#include <tic80.h>

static bool layer_need_redraw = true;

static void on_focus(void) { layer_need_redraw = true; }

static void tic(void) {
    if (layer_need_redraw) {
        // draw win screen
        FRAMEBUFFER->BORDER_COLOR_AND_OVR_TRANSPARENCY = COLOR_TABLE_BG;
        rect(0, 0, WIDTH, HEIGHT, COLOR_TABLE_BG);
        char text[] = "Congratulations! You win~";
        uint8_t offset_y = (HEIGHT - FONT_H) / 2;
        uint8_t text_w = print(text, 0, -FONT_H, COLOR_BLACK, false, 1, false);
        uint8_t offset_x = (WIDTH - text_w) / 2;
        print(text, offset_x, offset_y, COLOR_BLACK, false, 1, false);
    }
    layer_need_redraw = false;
    if (is_btn_pressed_once(BUTTON_CODE_P1_B)) {
        ui_pop_layer();
    }
}

static UILayerObject layer_object = {.on_focus = on_focus, .tic = tic};

UILayer layer_win = &layer_object;
