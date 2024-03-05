#include "freecell.h"
#include "ui_helper.h"
#include "ui_layers.h"
#include "ui_stack.h"
#include <stdio.h>

#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)
#define HISTORY_SIZE (64)
#define TILE_Y_CELL_TOP (1)
#define TILE_Y_UPPER_CURSOR (4)
#define TILE_Y_TABLE_TOP (5)
#define TILE_H_CARD (3)
#define MAP_BG_OFFX (30)
#define MAP_BG_OFFY (0)
#define MAP_FG_OFFX (60)
#define MAP_FG_OFFY (0)
#define MAP_BG_W (25)
#define MAP_BG_H (17)
#define MAP_FG_W (25)
#define MAP_FG_H (17)
#define MAP_OFFX_CARD_COL(x) (1 + ((x) * 3))
#define TILE_EMPTY (0)

typedef enum {
    EMP_NONE = 0,
    EMP_STYLE1 = 1, // border
    EMP_STYLE2 = 2, // border with circle
    CAD_AT_MID = 3, // card between others
} CardStyle;

static fc_Action history[HISTORY_SIZE];
static fc_Game game_obj = {
    .history_pointer = 0, .history_size = HISTORY_SIZE, .history = history};
static fc_Game *game = &game_obj;
static int8_t view_tile_start = 0; // 0 ~ 127
static int8_t cursor_position = 0; // 0 ~ 127
static int8_t token_position = -1; // 0 ~ 127

static void limit_view_position(bool locked_to_cursor) {
    uint8_t max_rows = fc_get_max_column_size(game);
    int8_t min_y = 0;
    if (locked_to_cursor) {
        if (cursor_position < fc_MOVE_FREE_CELL) {
            // cursor at table
            uint8_t col_size =
                fc_get_column_size(game, cursor_position - fc_MOVE_TABLE);
            int8_t cursor_top =
                TILE_Y_TABLE_TOP + TILE_H_CARD - 1 + (int8_t)col_size;
            if (col_size <= 0) {
                cursor_top = TILE_Y_TABLE_TOP;
            }
            min_y = cursor_top + 1 - HEIGHT_TILES;
            if (view_tile_start < min_y) {
                view_tile_start = min_y;
            }
            if (view_tile_start > cursor_top) {
                view_tile_start = cursor_top;
            }
        } else {
            // cursor at top
            view_tile_start = 0;
        }
    }
    view_tile_start = max(view_tile_start, 0);
    int8_t max_y =
        ((int16_t)max_rows + TILE_H_CARD + TILE_Y_TABLE_TOP - HEIGHT_TILES);
    max_y = max(max_y, 0);
    if ((view_tile_start > 0) && (max_y < view_tile_start)) {
        view_tile_start = max_y;
    }
}

static void map_clear(void) {
    for (int32_t iy = 0; iy < MAP_BG_H; iy++) {
        for (int32_t ix = 0; ix < MAP_BG_W; ix++) {
            mset(MAP_BG_OFFX + ix, MAP_BG_OFFY + iy, TILE_EMPTY);
        }
    }
    for (int32_t iy = 0; iy < MAP_FG_H; iy++) {
        for (int32_t ix = 0; ix < MAP_FG_W; ix++) {
            mset(MAP_FG_OFFX + ix, MAP_FG_OFFY + iy, TILE_EMPTY);
        }
    }
}

static void map_draw_card_top_at_column(int8_t map_row, fc_Card card,
                                        uint8_t column, CardStyle style) {
    int8_t col = MAP_OFFX_CARD_COL(column);
    if (card.card == fc_EMPTY_CARD) {
        if (style == EMP_STYLE1 || style == EMP_STYLE2) {
            mset(MAP_BG_OFFX + col, MAP_BG_OFFY + map_row, T_EMP_TOP);
            mset(MAP_BG_OFFX + col + 1, MAP_BG_OFFY + map_row, T_EMP_TOP + 1);
        }
    } else {
        // draw card
        int16_t bg_tile = T_CAD_TOP;
        if (style == CAD_AT_MID) {
            bg_tile = T_CAD_TOP_ALT;
        }
        mset(MAP_BG_OFFX + col, MAP_BG_OFFY + map_row, bg_tile);
        mset(MAP_BG_OFFX + col + 1, MAP_BG_OFFY + map_row, bg_tile + 1);
        uint8_t card_type = card.type;
        uint8_t card_value = card.value;
        int16_t value_tile = T_CAD_VAL0;
        if (card_type & 1) {
            value_tile = T_CAD_VAL1;
        }
        mset(MAP_FG_OFFX + col, MAP_BG_OFFY + map_row, T_CAD_TYPE + card_type);
        mset(MAP_FG_OFFX + col + 1, MAP_BG_OFFY + map_row,
             value_tile + card_value);
    }
}

static void map_draw_card_middle_at_column(int8_t map_row, fc_Card card,
                                           uint8_t column, CardStyle style) {
    int8_t col = MAP_OFFX_CARD_COL(column);
    if (card.card == fc_EMPTY_CARD) {
        if (style == EMP_STYLE1) {
            mset(MAP_BG_OFFX + col, MAP_BG_OFFY + map_row, T_EMP_MID);
            mset(MAP_BG_OFFX + col + 1, MAP_BG_OFFY + map_row, T_EMP_MID + 1);
        } else if (style == EMP_STYLE2) {
            mset(MAP_BG_OFFX + col, MAP_BG_OFFY + map_row, T_EMP_MID_ALT);
            mset(MAP_BG_OFFX + col + 1, MAP_BG_OFFY + map_row,
                 T_EMP_MID_ALT + 1);
        }
    } else {
        // draw card
        mset(MAP_BG_OFFX + col, MAP_BG_OFFY + map_row, T_CAD_MID);
        mset(MAP_BG_OFFX + col + 1, MAP_BG_OFFY + map_row, T_CAD_MID + 1);
    }
}

static void map_draw_card_bottom_at_column(int8_t map_row, fc_Card card,
                                           uint8_t column, CardStyle style) {
    int8_t col = MAP_OFFX_CARD_COL(column);
    if (card.card == fc_EMPTY_CARD) {
        if (style == EMP_STYLE1 || style == EMP_STYLE2) {
            mset(MAP_BG_OFFX + col, MAP_BG_OFFY + map_row, T_EMP_BOT);
            mset(MAP_BG_OFFX + col + 1, MAP_BG_OFFY + map_row, T_EMP_BOT + 1);
        }
    } else {
        // draw card
        mset(MAP_BG_OFFX + col, MAP_BG_OFFY + map_row, T_CAD_BOT);
        mset(MAP_BG_OFFX + col + 1, MAP_BG_OFFY + map_row, T_CAD_BOT + 1);
    }
}

static void draw_game() {
    // draw table
    FRAMEBUFFER->BORDER_COLOR_AND_OVR_TRANSPARENCY = COLOR_TABLE_BG;
    map_clear();
    rect(0, 0, WIDTH, HEIGHT, COLOR_TABLE_BG);
    uint8_t col_size[fc_COLUMN_COUNT];
    for (uint8_t col = 0; col < fc_COLUMN_COUNT; col++) {
        col_size[col] = fc_get_column_size(game, col);
    }
    for (int8_t row = view_tile_start; row < view_tile_start + HEIGHT_TILES;
         row++) {
        int8_t map_row = row - view_tile_start;
        if (row <= 0 || row == 4) {
            continue;
        } else if (row == 1 || row == 2 || row == 3) {
            // cells
            fc_Card card;
            for (uint8_t idx = 0; idx < fc_FREE_CELL_COUNT; idx++) {
                card = game->free_cell[idx];
                if (row == 1) {
                    map_draw_card_top_at_column(map_row, card, idx, EMP_STYLE1);
                } else if (row == 2) {
                    map_draw_card_middle_at_column(map_row, card, idx,
                                                   EMP_STYLE1);
                } else if (row == 3) {
                    map_draw_card_bottom_at_column(map_row, card, idx,
                                                   EMP_STYLE1);
                }
            }
            for (uint8_t idx = 0; idx < fc_COLLECT_CELL_COUNT; idx++) {
                card = game->collect_cell[idx];
                if (row == 1) {
                    map_draw_card_top_at_column(map_row, card, idx + 4,
                                                EMP_STYLE2);
                } else if (row == 2) {
                    map_draw_card_middle_at_column(map_row, card, idx + 4,
                                                   EMP_STYLE2);
                } else if (row == 3) {
                    map_draw_card_bottom_at_column(map_row, card, idx + 4,
                                                   EMP_STYLE2);
                }
            }
        } else {
            // table cards
            uint8_t card_row = row - TILE_Y_TABLE_TOP;
            for (uint8_t col = 0; col < fc_COLUMN_COUNT; col++) {
                fc_Card card = fc_get_card_at(game, col, card_row);
                fc_Card fake_card = {.type = 0, .value = 0};
                uint8_t last_mid_row = col_size[col];
                uint8_t last_bottom_row = col_size[col] + 1;
                if (card_row == 0) {
                    map_draw_card_top_at_column(map_row, card, col, EMP_NONE);
                } else if (card_row == last_mid_row) {
                    map_draw_card_middle_at_column(map_row, fake_card, col,
                                                   EMP_NONE);
                } else if (card_row == last_bottom_row) {
                    map_draw_card_bottom_at_column(map_row, fake_card, col,
                                                   EMP_NONE);
                } else {
                    map_draw_card_top_at_column(map_row, card, col, CAD_AT_MID);
                }
            }
        }
    }
    map(MAP_BG_OFFX, MAP_BG_OFFY, MAP_BG_W, MAP_BG_H, 0, 0, transcolor0, 1, 1,
        TIC80_PARAM_IGNORE);
    map(MAP_FG_OFFX, MAP_FG_OFFY, MAP_FG_W, MAP_FG_H, 0, 0, transcolor0, 1, 1,
        TIC80_PARAM_IGNORE);
    // draw cursor
    int8_t row = TILE_Y_UPPER_CURSOR;
    int8_t col = 0;
    if (cursor_position < fc_MOVE_FREE_CELL) {
        row = col_size[cursor_position - fc_MOVE_TABLE] - 1 + TILE_Y_TABLE_TOP +
              TILE_H_CARD - view_tile_start;
        col = cursor_position - fc_MOVE_TABLE;
    } else {
        row = TILE_Y_UPPER_CURSOR - view_tile_start;
        col = cursor_position - (int8_t)fc_MOVE_FREE_CELL;
    }
    col = 1 + ((col) * 3);
    if (col >= 0 && col < WIDTH_TILES && row >= 0 && row < HEIGHT_TILES) {
        spr(T_CUR2, col * TILE_W + (TILE_W / 2), row * TILE_H, transcolor0, 1,
            1, false, false, 1, 1);
    }
    // cursor 2
    if (token_position >= 0) {
        if (token_position < fc_MOVE_FREE_CELL) {
            row = col_size[token_position - fc_MOVE_TABLE] - 1 +
                  TILE_Y_TABLE_TOP + TILE_H_CARD - view_tile_start;
            col = token_position - fc_MOVE_TABLE;
        } else {
            row = TILE_Y_UPPER_CURSOR - view_tile_start;
            col = token_position - (int8_t)fc_MOVE_FREE_CELL;
        }
        col = 1 + ((col) * 3);
        if (col >= 0 && col < WIDTH_TILES && row >= 0 && row < HEIGHT_TILES) {
            spr(T_CUR3, col * TILE_W + (TILE_W / 2), row * TILE_H, transcolor0,
                1, 1, false, false, 1, 1);
        }
    }
}

void new_game_random(void) {
    // generate random seed
    uint32_t seed = tstamp();
    // init game
    fc_game_init(game, (int32_t)(seed & (uint32_t)INT32_MAX));
    fc_game_init(game, 0);
    view_tile_start = 0;
    cursor_position = 0;
    limit_view_position(false);
}

static void onFocus(void) {
    // generate random seed
    new_game_random();
    draw_game();
}

static void tic(void) {
    bool need_redraw = false;
    btn(TIC80_PARAM_IGNORE); // ?????
    // FIXME: btnp not good, need to use DMA or something else.
    // see https://github.com/nesbox/TIC-80/issues/2183
    if (btnp(BUTTON_CODE_P1_UP, TIC80_PARAM_IGNORE, TIC80_PARAM_IGNORE) & 0b1) {
        puts("up");
        need_redraw = true;
    }
    if (btnp(BUTTON_CODE_P1_DOWN, TIC80_PARAM_IGNORE, TIC80_PARAM_IGNORE) & 0b1) {
        puts("down");
        need_redraw = true;
    }
    if (btnp(BUTTON_CODE_P1_LEFT, TIC80_PARAM_IGNORE, TIC80_PARAM_IGNORE) & 0b1) {
        puts("left");
        if (cursor_position < fc_MOVE_FREE_CELL) {
            cursor_position += fc_COLUMN_COUNT - 1;
            cursor_position %= fc_COLUMN_COUNT;
        } else {
            cursor_position -= fc_MOVE_FREE_CELL;
            cursor_position += (fc_FREE_CELL_COUNT + fc_COLLECT_CELL_COUNT) - 1;
            cursor_position %= (fc_FREE_CELL_COUNT + fc_COLLECT_CELL_COUNT);
            cursor_position += fc_MOVE_FREE_CELL;
        }
        limit_view_position(true);
        need_redraw = true;
    }
    if (btnp(BUTTON_CODE_P1_RIGHT, TIC80_PARAM_IGNORE, TIC80_PARAM_IGNORE) & 0b1) {
        puts("right");
        if (cursor_position < fc_MOVE_FREE_CELL) {
            cursor_position += 1;
            cursor_position %= fc_COLUMN_COUNT;
        } else {
            cursor_position -= fc_MOVE_FREE_CELL;
            cursor_position += 1;
            cursor_position %= (fc_FREE_CELL_COUNT + fc_COLLECT_CELL_COUNT);
            cursor_position += fc_MOVE_FREE_CELL;
        }
        limit_view_position(true);
        need_redraw = true;
    }
    if (need_redraw) {
        draw_game();
    }
}

static UILayerObject layerObject = {.onFocus = onFocus, .tic = tic};

UILayer layer_freecell_game = &layerObject;