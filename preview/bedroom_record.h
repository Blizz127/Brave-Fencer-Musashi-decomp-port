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

/* The bag stores twelve item IDs. Their acquisition days are parallel
 * little-endian halfwords in the same variable bank. */
static inline int bfm_save_inventory_space(const BfmSaveRecord *record)
{
    if (record) {
        for (int slot = 0; slot < 12; ++slot)
            if (!record->variables[47 + slot]) return slot;
    }
    return -1;
}

static inline int bfm_save_add_item(BfmSaveRecord *record, unsigned item)
{
    int slot = bfm_save_inventory_space(record);
    if (slot < 0 || !item || item > 255) return 0;
    record->variables[47 + slot] = (uint8_t)item;
    record->variables[20 + 2 * slot] = record->progress[0x34];
    record->variables[21 + 2 * slot] = record->progress[0x35];
    return 1;
}

/* Bits 29..33 select alternate room tables on Continue. */
static inline int bfm_save_uses_default_room_table(const BfmSaveRecord *record)
{
    return !(record->flags[3] & 0xe0u) && !(record->flags[4] & 3u);
}

/* Default Continue uses the Grillin inn entrance throughout this story range. */
static inline int bfm_story_returns_to_grillin_inn(uint32_t story)
{
    return story >= 10 && story < 200;
}

typedef struct {
    BfmSaveRecord state;
    unsigned scene_member;
} BfmBedroomEntry;

/* Resolve the earned castle wake or the early Grillin inn exterior against
 * the mounted disc. Other saved destinations remain at the load boundary. */
int bfm_bedroom_entry(const MusashiDiscMedia *disc, const BfmSaveRecord *record,
                      BfmBedroomEntry *out);
int bfm_castle_room_entry(const MusashiDiscMedia *disc, const uint8_t *transfer,
                      BfmBedroomEntry *entry);
/* Validate the restored room without changing the checkpoint banks. For
 * a saved-data restart, SC03 member 3 owns the later exit to the village. */
int bfm_woods_restart_entry(const MusashiDiscMedia *disc, const BfmSaveRecord *record,
                            BfmBedroomEntry *out);
/* Mounted effect selector directory with its room-dependent sound rows. */
uint8_t *bfm_archive_effect_data(const MusashiDiscMedia *disc, const char *archive,
                                 unsigned member, size_t *size);
uint8_t *bfm_bedroom_effect_data(const MusashiDiscMedia *disc, unsigned member, size_t *size);
uint8_t *bfm_village_event_data(const MusashiDiscMedia *disc, unsigned member, size_t *size);
/* Loads the player body/motions from the resident MAIN member. Caller frees. */
uint8_t *bfm_bedroom_player_data(const MusashiDiscMedia *disc, size_t *size);
#endif
