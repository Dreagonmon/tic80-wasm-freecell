#include "ui_stack.h"
#include <stdint.h>
#include <stddef.h>
#include <tic80.h>

#define UI_STACK_SIZE (16)

static uint8_t ui_stack_top = 0;
static UILayer ui_stack[UI_STACK_SIZE];

bool ui_push_layer(UILayer layer) {
    if (ui_stack_top >= UI_STACK_SIZE) {
        return false;
    }
    if (ui_stack_top > 0) {
        UILayer last = ui_stack[ui_stack_top - 1];
        if (last->on_not_focus != NULL) {
            last->on_not_focus();
        }
    }
    ui_stack[ui_stack_top] = layer;
    ui_stack_top ++;
    if (layer->on_focus != NULL) {
        layer->on_focus();
    }
    return true;
}

bool ui_pop_layer(void) {
    if (ui_stack_top <= 0) {
        return false;
    }
    UILayer layer = ui_stack[ui_stack_top - 1];
    if (layer->on_not_focus != NULL) {
        layer->on_not_focus();
    }
    ui_stack_top --;
    if (ui_stack_top > 0) {
        UILayer last = ui_stack[ui_stack_top - 1];
        if (last->on_focus != NULL) {
            last->on_focus();
        }
    } else {
        // exit
        tic80_exit();
    }
    return true;
}

void ui_tic(void) {
    if (ui_stack_top > 0) {
        UILayer top = ui_stack[ui_stack_top - 1];
        if (top->tic != NULL) {
            top->tic();
        }
    }
}
