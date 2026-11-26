/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_RECORD_H
#define BFM_BEDROOM_RECORD_H

#include <stdint.h>
#include "musashi_disc_media.h"

/* The selected slot's 768-byte payload contains a 732-byte live record. */
typedef struct {
    uint8_t destination[36];
    uint8_t progress[152];
    uint8_t figure_prices[128];
    uint8_t flags[64];
    uint8_t variables[256];
    uint8_t timestamps[96];
} BfmSaveRecord;

typedef struct {
    BfmSaveRecord state;
    unsigned scene_member;
} BfmBedroomEntry;

/* Stage a checked Chapter 1 save at the story transition into room 3001.
 * Returns zero for another destination/story path or a malformed disc row. */
int bfm_bedroom_entry(const MusashiDiscMedia *disc, const BfmSaveRecord *record,
                      BfmBedroomEntry *out);
int bfm_library_entry(const MusashiDiscMedia *disc, const uint8_t *transfer,
                      BfmBedroomEntry *entry);
/* Loads the player body/motions from the resident MAIN member. Caller frees. */
uint8_t *bfm_bedroom_effect_data(const MusashiDiscMedia *disc, unsigned member, size_t *size);
uint8_t *bfm_bedroom_player_data(const MusashiDiscMedia *disc, size_t *size);
#endif
