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

/* Bits 29..33 select alternate room tables on Continue. */
static inline int bfm_save_uses_default_room_table(const BfmSaveRecord *record)
{
    return !(record->flags[3] & 0xe0u) && !(record->flags[4] & 3u);
}

typedef struct {
    BfmSaveRecord state;
    unsigned scene_member;
} BfmBedroomEntry;

/* Resolve the earned castle wake or the story-ten inn exterior against the
 * mounted disc. Unsupported saved destinations remain at the load boundary. */
int bfm_bedroom_entry(const MusashiDiscMedia *disc, const BfmSaveRecord *record,
                      BfmBedroomEntry *out);
int bfm_castle_room_entry(const MusashiDiscMedia *disc, const uint8_t *transfer,
                      BfmBedroomEntry *entry);
/* Mounted effect selector directory with its room-dependent sound rows. */
uint8_t *bfm_archive_effect_data(const MusashiDiscMedia *disc, const char *archive,
                                 unsigned member, size_t *size);
uint8_t *bfm_bedroom_effect_data(const MusashiDiscMedia *disc, unsigned member, size_t *size);
uint8_t *bfm_village_event_data(const MusashiDiscMedia *disc, unsigned member, size_t *size);
/* Loads the player body/motions from the resident MAIN member. Caller frees. */
uint8_t *bfm_bedroom_player_data(const MusashiDiscMedia *disc, size_t *size);
#endif
