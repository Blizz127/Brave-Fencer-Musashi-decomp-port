/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_OPENING_DATA_H
#define BFM_FOREST_OPENING_DATA_H
#include "forest_field.h"

typedef struct {
    uint32_t first_track, animation_table, message;
    int32_t initial_velocity;
    int16_t key[2][3];
} BfmForestOpeningData;
int bfm_forest_opening_data(const BfmForestField *field, BfmForestOpeningData *out);
/* Borrow a bounded word-aligned record from the current mounted room. */
const uint8_t *bfm_forest_room_bytes(const BfmForestField *field, uint32_t address,
                                     size_t length);
struct BfmForestMessageArt;
struct BfmForestMessageArt *bfm_forest_open_message_art(const BfmForestField *field,
                                                       const MusashiDiscMedia *disc);
#endif
