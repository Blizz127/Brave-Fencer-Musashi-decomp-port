/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_STATUS_H
#define BFM_FOREST_STATUS_H

#include "musashi_disc_media.h"

typedef struct {
    /* Mutable resident player record belonging to this New Game. */
    uint8_t record[152];
} BfmForestStatus;

typedef struct {
    uint32_t wallet;
    uint16_t max_hp, max_bp, day, minutes_of_day;
    int16_t hp, bp;
    uint8_t weekday, ability, ability_label;
    uint8_t charge, orb;
    uint16_t face_gauge;
} BfmForestStatusValues;

/* Read the resident New Game defaults from the mounted USA executable.
 * Initialize once for a new game, before the first forest HUD update. */
int bfm_forest_status_load(const MusashiDiscMedia *disc, BfmForestStatus *out);
/* Read the current values of a successfully initialized status record.
 * The wallet uses stored units. The HUD supplies its trailing zero. */
BfmForestStatusValues bfm_forest_status_values(const BfmForestStatus *status);
/* The grounded field stance samples BP, stamina, condition and ability together. */
int bfm_field_tired_stance(const uint8_t record[152]);
/* Resolve the ordinary bank rifle hit against the current equipped ability
 * and defense rank. The recipient applies the result when its hurt pose ends. */
int bfm_forest_hit_damage(const MusashiDiscMedia *disc, const BfmForestStatus *status,
                          uint32_t player_flags, unsigned power, uint16_t *amount);
int bfm_forest_rifle_damage(const MusashiDiscMedia *disc, const BfmForestStatus *status,
                            uint32_t player_flags, uint16_t *amount);
/* The summit acquisition restores both resources and equips Lumina. */
void bfm_forest_status_equip_lumina(BfmForestStatus *status);
void bfm_forest_status_defeated(BfmForestStatus *status);
/* Apply the early-chapter resource and wallet recovery. */
void bfm_forest_status_chapter_retry(BfmForestStatus *status);
void bfm_forest_status_debit(BfmForestStatus *status, unsigned amount);
void bfm_forest_status_spend_bp(BfmForestStatus *status, unsigned amount);

#endif
