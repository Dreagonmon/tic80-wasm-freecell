#ifndef UI_STACK_H
#define UI_STACK_H

#include <stdbool.h>

typedef void (*UIFunction)(void);

typedef struct {
    UIFunction onFocus;
    UIFunction onNotFocus;
    UIFunction tic;
} UILayerObject;
typedef UILayerObject *UILayer;

bool ui_push_layer(UILayer layer);
bool ui_pop_layer(void);
void ui_tic(void);

#endif
