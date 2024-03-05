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
        if (last->onNotFocus != NULL) {
            last->onNotFocus();
        }
    }
    ui_stack[ui_stack_top] = layer;
    ui_stack_top ++;
    if (layer->onFocus != NULL) {
        layer->onFocus();
    }
    return true;
}

bool ui_pop_layer(void) {
    if (ui_stack_top <= 0) {
        return false;
    }
    UILayer layer = ui_stack[ui_stack_top - 1];
    if (layer->onNotFocus != NULL) {
        layer->onNotFocus();
    }
    ui_stack_top --;
    if (ui_stack_top > 0) {
        UILayer last = ui_stack[ui_stack_top - 1];
        if (last->onFocus != NULL) {
            last->onFocus();
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
