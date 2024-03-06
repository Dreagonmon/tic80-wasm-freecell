#ifndef UI_STACK_H
#define UI_STACK_H

#include <stdbool.h>

typedef void (*UIFunction)(void);

typedef struct {
    UIFunction on_focus;
    UIFunction on_not_focus;
    UIFunction tic;
} UILayerObject;
typedef UILayerObject *UILayer;

bool ui_push_layer(UILayer layer);
bool ui_pop_layer(void);
void ui_tic(void);

#endif
