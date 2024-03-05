#ifndef FREECELL_H
#define FREECELL_H

#include <stddef.h>
#include <stdint.h>

/* Card */
#define fc_COLUMN_COUNT ((uint8_t)8)
#define fc_FREE_CELL_COUNT ((uint8_t)4)
#define fc_COLLECT_CELL_COUNT ((uint8_t)4)
#define fc_TYPE_COUNT ((uint8_t)4)
#define fc_VALUE_COUNT ((uint8_t)13)
#define fc_CARD_COUNT ((uint8_t)(fc_TYPE_COUNT * fc_VALUE_COUNT))
#define fc_EMPTY_CARD ((uint8_t)0xFF)
#define fc_TYPE_HEART ((uint8_t)0b00)
#define fc_TYPE_SPADE ((uint8_t)0b01)
#define fc_TYPE_DIAMOND ((uint8_t)0b10)
#define fc_TYPE_CLUB ((uint8_t)0b11)
#define fc_VALUE_1 ((uint8_t)0)
#define fc_VALUE_2 ((uint8_t)1)
#define fc_VALUE_3 ((uint8_t)2)
#define fc_VALUE_4 ((uint8_t)3)
#define fc_VALUE_5 ((uint8_t)4)
#define fc_VALUE_6 ((uint8_t)5)
#define fc_VALUE_7 ((uint8_t)6)
#define fc_VALUE_8 ((uint8_t)7)
#define fc_VALUE_9 ((uint8_t)8)
#define fc_VALUE_10 ((uint8_t)9)
#define fc_VALUE_JACK ((uint8_t)10)
#define fc_VALUE_QUEEN ((uint8_t)11)
#define fc_VALUE_KING ((uint8_t)12)

/* Movement */
#define fc_MOVE_TABLE ((uint8_t)0)
#define fc_MOVE_FREE_CELL ((uint8_t)8)
#define fc_MOVE_COLLECT_CELL ((uint8_t)12)
#define fc_TABLE(x) (fc_MOVE_TABLE + ((uint8_t)(x)))
#define fc_FREE(x) (fc_MOVE_FREE_CELL + ((uint8_t)(x)))
#define fc_COLLECT(x) (fc_MOVE_COLLECT_CELL + ((uint8_t)(x)))

/* Action */
#define fc_EMPTY_ACTION ((uint16_t)0xFFFF)

typedef union {
    struct {
        uint8_t value : 4;
        uint8_t type : 2;
        uint8_t flag1 : 1;
        uint8_t flag2 : 1;
    };
    uint8_t card;
} fc_Card;

typedef union {
    struct {
        uint8_t from : 4;
        uint8_t to : 4;
        uint8_t size : 4;
    };
    uint16_t action;
} fc_Action;

typedef struct {
    int32_t seed;
    fc_Card table[fc_CARD_COUNT];
    uint8_t column_tail[fc_COLUMN_COUNT];
    fc_Card free_cell[fc_FREE_CELL_COUNT];
    fc_Card collect_cell[fc_COLLECT_CELL_COUNT];
} fc_GameCorePart; // size reference

typedef struct {
    int32_t seed;
    fc_Card table[fc_CARD_COUNT];
    uint8_t column_tail[fc_COLUMN_COUNT];
    fc_Card free_cell[fc_FREE_CELL_COUNT];
    fc_Card collect_cell[fc_COLLECT_CELL_COUNT];
    uint8_t history_pointer;
    uint8_t history_size;
} fc_GameCorePartWithHistoryInfo; // size reference

typedef struct {
    int32_t seed;
    fc_Card table[fc_CARD_COUNT];
    uint8_t column_tail[fc_COLUMN_COUNT];
    fc_Card free_cell[fc_FREE_CELL_COUNT];
    fc_Card collect_cell[fc_COLLECT_CELL_COUNT];
    // history is always the last
    uint8_t history_pointer;
    uint8_t history_size;
    fc_Action *history; // history always the last element
} fc_Game;

fc_Game *fc_new_Game(uint8_t history_size);
void fc_free_Game(fc_Game *game);
void fc_game_init(fc_Game *game, int32_t rand_seed);
uint8_t fc_get_column_start_offset(fc_Game *game, uint8_t col);
uint8_t fc_get_column_size(fc_Game *game, uint8_t col);
uint8_t fc_get_max_column_size(fc_Game *game);
fc_Card fc_get_card_at(fc_Game *game, uint8_t col, uint8_t row);
uint8_t fc_move(fc_Game *game, uint8_t m_from, uint8_t m_to);
uint8_t fc_auto_collect(fc_Game *game);
void fc_undo(fc_Game *game);
uint16_t fc_get_save_size(uint8_t history_size);
uint16_t fc_save_game(fc_Game *game, void *buf, uint16_t len);
uint16_t fc_load_game(void *buf, uint16_t len, fc_Game *game);

#endif
