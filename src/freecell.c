#include "freecell.h"
#include <stdbool.h>
#include <stdlib.h>

#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

int fc_random_int(int32_t xn) {
    // return [0, 2**31)
    return ((int64_t)1103515245 * (int64_t)xn + (int64_t)12345) & INT32_MAX;
}

fc_Game *fc_new_Game(uint8_t history_size) {
    fc_Game *game = malloc(sizeof(fc_Game));
    game->history = NULL;
    game->history_pointer = 0;
    game->history_size = 0;
    if (game != NULL && history_size > 0) {
        fc_Action *history = malloc(sizeof(fc_Action) * history_size);
        if (history != NULL) {
            game->history = history;
            game->history_pointer = 0;
            game->history_size = history_size;
        } else {
            free(game);
            game = NULL;
        }
    }
    return game;
}

void fc_free_Game(fc_Game *game) {
    if (game == NULL) {
        return;
    }
    if (game->history != NULL) {
        free(game->history);
    }
    free(game);
}

void fc_game_init(fc_Game *game, int32_t rand_seed) {
    game->seed = rand_seed;
    for (int i = 0; i < fc_FREE_CELL_COUNT; i++) {
        game->free_cell[i].card = fc_EMPTY_CARD;
    }
    for (int i = 0; i < fc_COLLECT_CELL_COUNT; i++) {
        game->collect_cell[i].card = fc_EMPTY_CARD;
    }
    for (int typ = 0; typ < fc_TYPE_COUNT; typ++) {
        for (int val = 0; val < fc_VALUE_COUNT; val++) {
            int idx = typ * fc_VALUE_COUNT + val;
            game->table[idx].type = typ;
            game->table[idx].value = val;
        }
    }
    uint8_t cards_per_column = fc_CARD_COUNT / fc_COLUMN_COUNT;
    uint8_t more_cards = fc_CARD_COUNT % fc_COLUMN_COUNT;
    uint8_t card_count = 0;
    for (uint8_t col = 0; col < fc_COLUMN_COUNT; col++) {
        card_count += cards_per_column;
        if (more_cards > 0) {
            card_count++;
            more_cards--;
        }
        game->column_tail[col] = card_count;
    }
    for (int i = 0; i < fc_CARD_COUNT; i++) {
        fc_Card curr = game->table[i];
        rand_seed = fc_random_int(rand_seed);
        int idx = rand_seed % fc_CARD_COUNT;
        game->table[i] = game->table[idx];
        game->table[idx] = curr;
    }
}

uint8_t fc_get_column_start_offset(fc_Game *game, uint8_t col) {
    uint8_t start = 0;
    if (col > 0) {
        start = game->column_tail[col - 1];
    }
    return start;
}

uint8_t fc_get_column_size(fc_Game *game, uint8_t col) {
    return game->column_tail[col] - fc_get_column_start_offset(game, col);
}

uint8_t fc_get_max_column_size(fc_Game *game) {
    uint8_t max = 0;
    for (uint8_t col = 0; col < fc_COLUMN_COUNT; col++) {
        uint8_t col_size = fc_get_column_size(game, col);
        max = (max < col_size) ? col_size : max;
    }
    return max;
}

fc_Card fc_get_card_at(fc_Game *game, uint8_t col, uint8_t row) {
    if (col >= fc_COLUMN_COUNT || row >= fc_CARD_COUNT) {
        return ((fc_Card){.card = fc_EMPTY_CARD});
    }
    uint8_t start = fc_get_column_start_offset(game, col);
    uint8_t size = game->column_tail[col] - start;
    if (row >= size) {
        return ((fc_Card){.card = fc_EMPTY_CARD});
    }
    return game->table[start + row];
}

static void fc_move_between_columns(fc_Game *game, uint8_t col_from,
                                    uint8_t col_to, uint8_t count) {
    // move in the table
    //   |  s f       t
    //   ||---|---|---|---|
    //   |    t     s f
    uint8_t count_limit = fc_get_column_size(game, col_from);
    uint8_t f_end = game->column_tail[col_from];
    uint8_t t_end = game->column_tail[col_to];
    fc_Card cards[fc_VALUE_COUNT];
    count = min(count, count_limit);
    count = min(count, fc_VALUE_COUNT); // maybe useless check
    if (col_from < col_to) {
        uint8_t p0 = f_end - count;
        uint8_t p1 = t_end - count;
        // tmp cards
        for (int i = 0; i < count; i++) {
            cards[i] = game->table[p0 + i];
        }
        // move backward
        for (int i = p0; i < p1; i++) {
            game->table[i] = game->table[i + count];
        }
        for (int i = 0; i < count; i++) {
            game->table[p1 + i] = cards[i];
        }
        // modify column tail
        for (int i = col_from; i < col_to; i++) {
            game->column_tail[i] -= count;
        }
    } else {
        uint8_t p0 = f_end - count;
        uint8_t p1 = t_end + count;
        // tmp cards
        for (int i = 0; i < count; i++) {
            cards[i] = game->table[p0 + i];
        }
        // move forward
        for (int i = f_end - 1; i >= p1; i--) {
            game->table[i] = game->table[i - count];
        }
        for (int i = 0; i < count; i++) {
            game->table[t_end + i] = cards[i];
        }
        // modify column tail
        for (int i = col_to; i < col_from; i++) {
            game->column_tail[i] += count;
        }
    }
}

static fc_Card fc_remove_1_from_column(fc_Game *game, uint8_t col) {
    uint8_t start = fc_get_column_start_offset(game, col);
    uint8_t size = game->column_tail[col] - start;
    if (size <= 0) {
        return ((fc_Card){.card = fc_EMPTY_CARD});
    }
    uint8_t p0 = game->column_tail[col] - 1;
    uint8_t p1 = game->column_tail[7] - 1;
    fc_Card card = game->table[p0];
    // move backward
    for (int i = p0; i < p1; i++) {
        game->table[i] = game->table[i + 1];
    }
    game->table[p1].card = fc_EMPTY_CARD;
    // modify column tail
    for (int i = col; i < fc_COLUMN_COUNT; i++) {
        game->column_tail[i] -= 1;
    }
    return card;
}

static fc_Card fc_add_1_to_column(fc_Game *game, uint8_t col, fc_Card card) {
    if (game->column_tail[7] >= fc_CARD_COUNT) {
        return ((fc_Card){.card = fc_EMPTY_CARD});
    }
    uint8_t p0 = game->column_tail[col] + 1;
    // move forward
    for (int i = game->column_tail[7]; i >= p0; i--) {
        game->table[i] = game->table[i - 1];
    }
    game->table[game->column_tail[col]] = card;
    // modify column tail
    for (int i = col; i < fc_COLUMN_COUNT; i++) {
        game->column_tail[i] += 1;
    }
    return card;
}

static fc_Card fc_remove_1_from_collect(fc_Game *game, uint8_t idx) {
    if (idx >= fc_COLLECT_CELL_COUNT ||
        game->collect_cell[idx].card == fc_EMPTY_CARD) {
        return ((fc_Card){.card = fc_EMPTY_CARD});
    }
    fc_Card card = game->collect_cell[idx];
    if (card.value <= 0) {
        game->collect_cell[idx].card = fc_EMPTY_CARD;
    } else {
        game->collect_cell[idx].value--;
    }
    return card;
}

static fc_Card fc_add_1_to_collect(fc_Game *game, uint8_t idx, fc_Card card) {
    if (idx >= fc_COLLECT_CELL_COUNT) {
        return ((fc_Card){.card = fc_EMPTY_CARD});
    }
    if (game->collect_cell[idx].card == fc_EMPTY_CARD) {
        // any type, value == 0
        if (card.value != 0) {
            return ((fc_Card){.card = fc_EMPTY_CARD});
        }
    } else {
        // same type, value == +1
        if (card.type != game->collect_cell[idx].type) {
            return ((fc_Card){.card = fc_EMPTY_CARD});
        }
        if (card.value != (game->collect_cell[idx].value + 1)) {
            return ((fc_Card){.card = fc_EMPTY_CARD});
        }
    }
    game->collect_cell[idx] = card;
    return card;
}

static fc_Card fc_remove_1_from_free(fc_Game *game, uint8_t idx) {
    if (idx >= fc_FREE_CELL_COUNT ||
        game->free_cell[idx].card == fc_EMPTY_CARD) {
        return ((fc_Card){.card = fc_EMPTY_CARD});
    }
    fc_Card card = game->free_cell[idx];
    game->free_cell[idx].card = fc_EMPTY_CARD;
    return card;
}

static fc_Card fc_add_1_to_free(fc_Game *game, uint8_t idx, fc_Card card) {
    if (idx >= fc_FREE_CELL_COUNT) {
        return ((fc_Card){.card = fc_EMPTY_CARD});
    }
    game->free_cell[idx] = card;
    return card;
}

static uint32_t fc_max_moveable_cards(fc_Game *game, uint8_t target_col) {
    uint32_t empty_columns_mux = 1;
    for (uint8_t col = 0; col < fc_COLUMN_COUNT; col++) {
        if (col == target_col) {
            continue;
        }
        uint8_t col_size = fc_get_column_size(game, col);
        if (col_size <= 0) {
            empty_columns_mux *= 2;
        }
    }
    uint32_t free_cells = 0;
    for (uint8_t i = 0; i < fc_FREE_CELL_COUNT; i++) {
        if (game->free_cell[i].card == fc_EMPTY_CARD) {
            free_cells++;
        }
    }
    return (free_cells + 1) * empty_columns_mux;
}

static void fc_do_action(fc_Game *game, fc_Action action) {
    // execute action, do not check.
    if (action.from >= fc_MOVE_TABLE && action.from < fc_MOVE_FREE_CELL &&
        action.to >= fc_MOVE_TABLE && action.to < fc_MOVE_FREE_CELL) {
        // from table to table
        uint8_t tcol = action.to - fc_MOVE_TABLE;
        uint8_t fcol = action.from - fc_MOVE_TABLE;
        fc_move_between_columns(game, fcol, tcol, action.size);
    } else {
        fc_Card card = ((fc_Card){.card = fc_EMPTY_CARD});
        // get 1 card
        if (action.from >= fc_MOVE_COLLECT_CELL) {
            // from collect cell
            uint8_t fidx = action.from - fc_MOVE_COLLECT_CELL;
            card = fc_remove_1_from_collect(game, fidx);
        } else if (action.from >= fc_MOVE_FREE_CELL) {
            // from free cell
            uint8_t fidx = action.from - fc_MOVE_FREE_CELL;
            card = fc_remove_1_from_free(game, fidx);
        } else if (action.from >= fc_MOVE_TABLE) {
            // from table
            uint8_t fcol = action.from - fc_MOVE_TABLE;
            card = fc_remove_1_from_column(game, fcol);
        }
        // put 1 card
        if (action.to >= fc_MOVE_COLLECT_CELL) {
            // to collect cell
            uint8_t tidx = action.to - fc_MOVE_COLLECT_CELL;
            fc_add_1_to_collect(game, tidx, card);
        } else if (action.to >= fc_MOVE_FREE_CELL) {
            // to free cell
            uint8_t tidx = action.to - fc_MOVE_FREE_CELL;
            fc_add_1_to_free(game, tidx, card);
        } else if (action.to >= fc_MOVE_TABLE) {
            // to table
            uint8_t tcol = action.to - fc_MOVE_TABLE;
            fc_add_1_to_column(game, tcol, card);
        }
    }
}

static bool fc_card_is_continue_table(fc_Card upper, fc_Card lower) {
    // different color, and value is continue
    return (((upper.type ^ lower.type) & 0b1) == 0b1) &&
           (upper.value == (lower.value + 1));
}

static bool fc_card_is_continue_collect(fc_Card upper, fc_Card lower) {
    if (upper.card == fc_EMPTY_CARD && lower.value == 0) {
        return true; // place at a empty cell
    }
    return (upper.type == lower.type) && ((upper.value + 1) == lower.value);
}

static void fc_push_history(fc_Game *game, fc_Action action) {
    if (game->history != NULL) {
        if (game->history_pointer < game->history_size) {
            game->history[game->history_pointer] = action;
            (game->history_pointer)++;
        } else {
            // move backward
            uint8_t current = 1;
            while (current < game->history_size) {
                game->history[current - 1] = game->history[current];
                current++;
            }
            // push history
            game->history[game->history_size - 1] = action;
        }
    }
}

static fc_Action fc_pop_history(fc_Game *game) {
    fc_Action action = ((fc_Action){.action = fc_EMPTY_ACTION});
    if (game->history != NULL) {
        if (game->history_pointer > 0) {
            (game->history_pointer)--;
            action = game->history[game->history_pointer];
        }
    }
    return action;
}

uint8_t fc_move(fc_Game *game, uint8_t m_from, uint8_t m_to) {
    // check to
    fc_Card target_card = ((fc_Card){.card = fc_EMPTY_CARD});
    fc_Card source_card = ((fc_Card){.card = fc_EMPTY_CARD});
    fc_Action action = ((fc_Action){.action = fc_EMPTY_ACTION});
    if (m_to >= fc_MOVE_COLLECT_CELL) {
        // to collect cell
        uint8_t tidx = m_to - fc_MOVE_COLLECT_CELL;
        target_card = game->collect_cell[tidx];
        if (m_from >= fc_MOVE_FREE_CELL && m_from < fc_MOVE_COLLECT_CELL) {
            // from free cell
            uint8_t fidx = m_from - fc_MOVE_FREE_CELL;
            source_card = game->free_cell[fidx];
        } else if (m_from >= fc_MOVE_TABLE) {
            // from table
            uint8_t fcol = m_from - fc_MOVE_TABLE;
            uint8_t size = fc_get_column_size(game, fcol);
            if (size > 0) {
                source_card = game->table[game->column_tail[fcol] - 1];
            }
        } // else useless move
        // build action
        if (source_card.card != fc_EMPTY_CARD) {
            if (((target_card.card == fc_EMPTY_CARD) &&
                 (source_card.value == 0)) ||
                ((target_card.card != fc_EMPTY_CARD) &&
                 (source_card.value == (target_card.value + 1)) &&
                 (source_card.type == target_card.type))) {
                action.from = m_from;
                action.to = m_to;
                action.size = 1;
            }
        }
    } else if (m_to >= fc_MOVE_FREE_CELL) {
        // to free cell
        uint8_t tidx = m_to - fc_MOVE_FREE_CELL;
        target_card = game->free_cell[tidx];
        if (m_from >= fc_MOVE_FREE_CELL && m_from < fc_MOVE_COLLECT_CELL) {
            // from free cell
            uint8_t fidx = m_from - fc_MOVE_FREE_CELL;
            source_card = game->free_cell[fidx];
        } else if (m_from >= fc_MOVE_TABLE) {
            // from table
            uint8_t fcol = m_from - fc_MOVE_TABLE;
            uint8_t size = fc_get_column_size(game, fcol);
            if (size > 0) {
                source_card = game->table[game->column_tail[fcol] - 1];
            }
        } // else useless move
        // build action
        if ((target_card.card == fc_EMPTY_CARD) &&
            (source_card.card != fc_EMPTY_CARD)) {
            action.from = m_from;
            action.to = m_to;
            action.size = 1;
        }
    } else if (m_to >= fc_MOVE_TABLE) {
        // to table
        uint8_t tcol = m_to - fc_MOVE_TABLE;
        uint8_t size = fc_get_column_size(game, tcol);
        if (size > 0) {
            target_card = game->table[game->column_tail[tcol] - 1];
        }
        if (m_from >= fc_MOVE_FREE_CELL && m_from < fc_MOVE_COLLECT_CELL) {
            // from free cell
            uint8_t fidx = m_from - fc_MOVE_FREE_CELL;
            source_card = game->free_cell[fidx];
            if (source_card.card != fc_EMPTY_CARD) {
                if (target_card.card == fc_EMPTY_CARD) {
                    action.from = m_from;
                    action.to = m_to;
                    action.size = 1;
                } else {
                    if (fc_card_is_continue_table(target_card, source_card)) {
                        // different color, value = +1
                        action.from = m_from;
                        action.to = m_to;
                        action.size = 1;
                    }
                }
            }
        } else if (m_from >= fc_MOVE_TABLE) {
            // from table
            uint8_t fcol = m_from - fc_MOVE_TABLE;
            uint8_t fstart = fc_get_column_start_offset(game, fcol);
            // find suitable card
            uint32_t max_move_size = fc_max_moveable_cards(game, tcol);
            uint8_t action_size = 0;
            fc_Card last_card = ((fc_Card){.card = fc_EMPTY_CARD});
            for (uint8_t p = game->column_tail[fcol] - 1; p >= fstart; p--) {
                action_size = game->column_tail[fcol] - p;
                if (action_size > max_move_size) {
                    break; // not enough free cell
                }
                source_card = game->table[p];
                // check card is continue
                if (last_card.card != fc_EMPTY_CARD) {
                    if (!fc_card_is_continue_table(source_card, last_card)) {
                        break; // value not continue
                    }
                }
                last_card = source_card;
                // check if fit the target slot
                if (target_card.card == fc_EMPTY_CARD) {
                    // target is empty, just update action size and try more
                    // card.
                    action.from = m_from;
                    action.to = m_to;
                    action.size = action_size;
                } else if (fc_card_is_continue_table(target_card,
                                                     source_card)) {
                    // this card fit the target card
                    action.from = m_from;
                    action.to = m_to;
                    action.size = action_size;
                    break;
                }
            }
        } // else useless move
    }
    // execute action
    if (action.action != fc_EMPTY_ACTION) {
        fc_do_action(game, action);
        fc_push_history(game, action);
        return action.size;
    }
    return 0;
}

uint8_t fc_auto_collect(fc_Game *game) {
    // check each collect cell
    for (uint8_t idx = 0; idx < fc_COLLECT_CELL_COUNT; idx++) {
        fc_Card target_card = game->collect_cell[idx];
        // check every column
        for (uint8_t col = 0; col < fc_COLUMN_COUNT; col++) {
            // get source card
            fc_Card source_card = ((fc_Card){.card = fc_EMPTY_CARD});
            uint8_t size = fc_get_column_size(game, col);
            if (size > 0) {
                source_card = game->table[game->column_tail[col] - 1];
            }
            // check cards
            if (fc_card_is_continue_collect(target_card, source_card)) {
                fc_Action action = ((fc_Action){.action = fc_EMPTY_ACTION});
                action.from = fc_TABLE(col);
                action.to = fc_COLLECT(idx);
                action.size = 1;
                fc_do_action(game, action);
                fc_push_history(game, action);
                return action.size;
            }
        }
    }
    return 0;
}

void fc_undo(fc_Game *game) {
    fc_Action action = fc_pop_history(game);
    if (action.action != fc_EMPTY_ACTION) {
        uint8_t tmp = action.to;
        action.to = action.from;
        action.from = tmp;
        fc_do_action(game, action);
    }
}

uint16_t fc_get_save_size(uint8_t history_size) {
    uint8_t size = sizeof(fc_GameCorePartWithHistoryInfo);
    return size + (history_size * sizeof(fc_Action));
}

uint16_t fc_save_game(fc_Game *game, void *buf, uint16_t len) {
    uint16_t game_size = sizeof(fc_GameCorePartWithHistoryInfo);
    if (len < (game_size + (game->history_size * sizeof(fc_Action)))) {
        return 0;
    }
    // copy game
    uint8_t *src = (uint8_t *)game;
    uint8_t *dest = (uint8_t *)buf;
    for (uint16_t i = 0; i < game_size; i++) {
        dest[i] = src[i];
    }
    // copy history
    src = (uint8_t *)game->history;
    for (uint16_t i = 0; i < (game->history_size * sizeof(fc_Action)); i++) {
        dest[game_size + i] = src[i];
    }
    return game_size + (game->history_size * sizeof(fc_Action));
}

uint16_t fc_load_game(void *buf, uint16_t len, fc_Game *game) {
    uint16_t game_size = sizeof(fc_GameCorePartWithHistoryInfo);
    uint16_t game_size_without_history = sizeof(fc_GameCorePart);
    if (len < game_size) {
        return 0;
    }
    // copy game without history
    uint8_t *src = (uint8_t *)buf;
    uint8_t *dest = (uint8_t *)game;
    for (uint16_t i = 0; i < game_size_without_history; i++) {
        dest[i] = src[i];
    }
    // copy history
    uint8_t buf_history_size =
        ((fc_GameCorePartWithHistoryInfo *)buf)->history_pointer;
    uint8_t *buf_history = src + game_size;
    uint16_t history_size =
        (len - game_size) / sizeof(fc_Action); // max buf size
    history_size = min(history_size, buf_history_size);
    game->history_pointer = 0; // reset game history
    for (uint16_t i = 0; i < history_size; i++) {
        fc_Action action = ((fc_Action *)buf_history)[i];
        fc_push_history(game, action);
    }
    return game_size + (history_size * sizeof(fc_Action));
}
