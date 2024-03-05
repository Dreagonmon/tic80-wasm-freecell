#include "ui_layers.h"
#include "ui_stack.h"
#include <_malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <tic80.h>

void init_heap(void) {
    size_t mems = _init_memory(4);
    printf("heap: %lu bytes\n", mems);
}

WASM_EXPORT("BOOT")
void BOOT() {
    init_heap(); // if you don't need malloc functions, you can skip init_heap()
    FRAMEBUFFER->BORDER_COLOR_AND_OVR_TRANSPARENCY = 0; // set border color
    // push title screen layer
    ui_push_layer(layer_title_screen);
}

WASM_EXPORT("TIC")
void TIC() { ui_tic(); }
