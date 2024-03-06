#include "freecell.h"
#include "ui_helper.h"
#include "ui_layers.h"
#include "ui_stack.h"
#include <stdio.h>

#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)
#define HISTORY_SIZE (80)
#define SAVE_SIZE (256)
#if ((76 + (HISTORY_SIZE * 2)) > SAVE_SIZE)
#error "SAVE_SIZE is too small, or HISTORY_SIZE is too big"
#endif
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
#define TILE_X_MENU_LEFT max(MAP_BG_W, MAP_FG_W)
#define TILE_W_MENU (WIDTH_TILES - TILE_X_MENU_LEFT)
#define MENU_OFFX (TILE_W * TILE_X_MENU_LEFT)
#define MENU_W (TILE_W * TILE_W_MENU)
#define SAVE_SLOT_COUNT (PERSISTENT_MEMORY_SIZE / SAVE_SIZE)
#define MENU_SIZE ((SAVE_SLOT_COUNT * 2) + 1)

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
static uint8_t ui_status = 255; // 0:normal 1:after_move 2:menu 255:need_redraw
static uint8_t delay_ticks = 0;
static int8_t menu_position = 0;

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
                } else if (card_row == last_mid_row && col_size[col] > 0) {
                    map_draw_card_middle_at_column(map_row, fake_card, col,
                                                   EMP_NONE);
                } else if (card_row == last_bottom_row && col_size[col] > 0) {
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
    if (ui_status == 0) {
        // draw cursor
        int8_t row = TILE_Y_UPPER_CURSOR;
        int8_t col = 0;
        if (cursor_position != token_position) {
            if (cursor_position < fc_MOVE_FREE_CELL) {
                row = col_size[cursor_position - fc_MOVE_TABLE] - 1 +
                      TILE_Y_TABLE_TOP + TILE_H_CARD - view_tile_start;
                col = cursor_position - fc_MOVE_TABLE;
            } else {
                row = TILE_Y_UPPER_CURSOR - view_tile_start;
                col = cursor_position - (int8_t)fc_MOVE_FREE_CELL;
            }
            col = 1 + ((col) * 3);
            if (col >= 0 && col < WIDTH_TILES && row >= 0 &&
                row < HEIGHT_TILES) {
                spr(T_CUR2, col * TILE_W + (TILE_W / 2), row * TILE_H,
                    transcolor0, 1, 1, false, false, 1, 1);
            }
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
            if (col >= 0 && col < WIDTH_TILES && row >= 0 &&
                row < HEIGHT_TILES) {
                spr(T_CUR3, col * TILE_W + (TILE_W / 2), row * TILE_H,
                    transcolor0, 1, 1, false, false, 1, 1);
            }
        }
    }
}

static void draw_menu(void) {
    uint8_t offset_y = 0;
    char line[20];
    // seed
    print("seed: ", MENU_OFFX, 0, COLOR_BLACK, false, 1, false);
    offset_y += FONT_H;
    sprintf(line, "%d", game->seed);
    uint8_t width = print(line, 0, -FONT_H, COLOR_BLACK, true, 1, true);
    uint8_t offset_x = (MENU_W - width) + MENU_OFFX;
    print(line, offset_x, offset_y, COLOR_BLACK, true, 1, true);
    offset_y += FONT_H;
    // menu items
    offset_x = MENU_OFFX + TILE_W;
    offset_y = (HEIGHT - (FONT_H * 4) - (MENU_SIZE * FONT_H)) / 2;
    for (uint8_t save_slot = 0; save_slot < SAVE_SLOT_COUNT; save_slot ++) {
        sprintf(line, "SAVE%1u", save_slot);
        print(line, offset_x, offset_y, COLOR_BLACK, false, 1, false);
        offset_y += FONT_H;
    }
    for (uint8_t save_slot = 0; save_slot < SAVE_SLOT_COUNT; save_slot ++) {
        sprintf(line, "LOAD%1u", save_slot);
        print(line, offset_x, offset_y, COLOR_BLACK, false, 1, false);
        offset_y += FONT_H;
    }
    print("QUIT", offset_x, offset_y, COLOR_BLACK, false, 1, false);
    // help text
    offset_y = HEIGHT - FONT_H - FONT_H;
    print("X: UNDO", MENU_OFFX, offset_y, COLOR_BLACK, false, 1, false);
    offset_y += FONT_H;
    print("Y: MENU", MENU_OFFX, offset_y, COLOR_BLACK, false, 1, false);
    // cursor
    if (ui_status == 2) {
        menu_position %= MENU_SIZE;
        offset_y = (HEIGHT - (FONT_H * 4) - (MENU_SIZE * FONT_H)) / 2;
        offset_y += menu_position * FONT_H - 2;
        spr(T_CUR2 + 3, MENU_OFFX, offset_y, transcolor0, 1, 1, false, false,
        1, 1);
    }
}

static void draw(void) {
    draw_game();
    draw_menu();
}

static void new_game_random(void) {
    // generate random seed
    uint32_t seed = fc_random_int(tstamp());
    // init game
    fc_game_init(game, (int32_t)(seed & (uint32_t)INT32_MAX));
    view_tile_start = 0;
    cursor_position = 0;
    menu_position = 0;
    limit_view_position(false);
}

static void save_to_slot(uint8_t slot) {
    uint8_t *save_memory = PERSISTENT_MEMORY + (SAVE_SIZE * slot);
    fc_save_game(game, save_memory, SAVE_SIZE);
}

static void load_from_slot(uint8_t slot) {
    uint8_t *save_memory = PERSISTENT_MEMORY + (SAVE_SIZE * slot);
    fc_load_game(save_memory, SAVE_SIZE, game);
}

static bool process_game_button(void) {
    bool need_redraw = false;
    // btnp() not good, need to use DMA or something else.
    // see https://github.com/nesbox/TIC-80/issues/2183
    if (is_btn_pressed_once(BUTTON_CODE_P1_UP)) {
        // move view up
        view_tile_start -= 4;
        view_tile_start = max(view_tile_start, 0);
        // move cursor up
        if (view_tile_start <= TILE_Y_UPPER_CURSOR &&
            cursor_position < fc_MOVE_FREE_CELL) {
            cursor_position += fc_MOVE_FREE_CELL;
        }
        need_redraw = true;
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_DOWN)) {
        // move cursor down
        uint8_t new_cursor_position = cursor_position + fc_MOVE_FREE_CELL;
        new_cursor_position %= fc_MOVE_FREE_CELL;
        uint8_t cursor_col = new_cursor_position - fc_MOVE_TABLE;
        int8_t cursor_tile_row = fc_get_column_size(game, cursor_col) - 1 +
                                 TILE_Y_TABLE_TOP + TILE_H_CARD -
                                 view_tile_start;

        if (cursor_position >= fc_MOVE_FREE_CELL && cursor_tile_row >= 0 &&
            // new cursor in view, just move it
            cursor_tile_row < HEIGHT_TILES) {
            cursor_position = new_cursor_position;
        } else {
            // move view down
            uint8_t max_rows = fc_get_max_column_size(game);
            int8_t max_y = ((int16_t)max_rows + TILE_H_CARD + TILE_Y_TABLE_TOP -
                            HEIGHT_TILES);
            view_tile_start += 4;
            view_tile_start = min(view_tile_start, max_y);
        }
        limit_view_position(false);
        need_redraw = true;
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_LEFT)) {
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
    if (is_btn_pressed_once(BUTTON_CODE_P1_RIGHT)) {
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
    if (is_btn_pressed_once(BUTTON_CODE_P1_A)) {
        if (token_position < 0) {
            if (cursor_position < fc_MOVE_FREE_CELL) {
                if (fc_get_column_size(game, cursor_position - fc_MOVE_TABLE) >
                    0) {
                    token_position = cursor_position;
                }
            } else if (cursor_position < fc_MOVE_COLLECT_CELL) {
                if (game->free_cell[cursor_position - fc_MOVE_FREE_CELL].card !=
                    fc_EMPTY_CARD) {
                    token_position = cursor_position;
                }
            }
        } else {
            if (cursor_position != token_position) {
                fc_move(game, (uint8_t)token_position,
                        (uint8_t)cursor_position);
                ui_status = 1;
                delay_ticks = 0;
                token_position = -1;
            }
        }
        need_redraw = true;
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_B)) {
        if (token_position >= 0) {
            token_position = -1;
            need_redraw = true;
        }
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_X)) {
        fc_undo(game);
        need_redraw = true;
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_Y)) {
        ui_status = 2;
        need_redraw = true;
    }
    return need_redraw;
}

static bool process_menu_button(void) {
    bool need_redraw = false;
    if (is_btn_pressed_once(BUTTON_CODE_P1_UP)) {
        menu_position += MENU_SIZE - 1;
        menu_position %= MENU_SIZE;
        need_redraw = true;
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_DOWN)) {
        menu_position += 1;
        menu_position %= MENU_SIZE;
        need_redraw = true;
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_A)) {
        ui_status = 0;
        need_redraw = true;
        if (menu_position == MENU_SIZE - 1) {
            ui_pop_layer();
            need_redraw = false;
        } else if (menu_position >= SAVE_SLOT_COUNT) {
            // load
            load_from_slot(menu_position - SAVE_SLOT_COUNT);
        } else {
            // save
            save_to_slot(menu_position);
        }
    }
    if (is_btn_pressed_once(BUTTON_CODE_P1_B)) {
        ui_status = 0;
        need_redraw = true;
    }
    return need_redraw;
}

static void on_focus_random(void) {
    // generate random seed
    new_game_random();
}

static void on_not_focus(void) { ui_status = 255; }

static void tic(void) {
    bool need_redraw = false;
    if (ui_status == 0) {
        if (process_game_button()) {
            need_redraw = true;
        }
    } else if (ui_status == 1) {
        if (delay_ticks >= 15) {
            if (!fc_auto_collect(game)) {
                ui_status = 0;
            }
            need_redraw = true;
            delay_ticks = 0;
        }
        delay_ticks++;
    } else if (ui_status == 2) {
        if (process_menu_button()) {
            need_redraw = true;
        }
    } else if (ui_status == 255) {
        need_redraw = true;
        ui_status = 0;
    }
    if (need_redraw) {
        draw();
    }
}

static UILayerObject layer_random_object = {
    .on_focus = on_focus_random, .tic = tic, .on_not_focus = on_not_focus};

UILayer layer_random_freecell_game = &layer_random_object;