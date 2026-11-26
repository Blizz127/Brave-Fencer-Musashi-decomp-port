/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BOULDERS_H
#define BFM_FOREST_BOULDERS_H
#include "forest_barrier_art.h"
#include "forest_guard_contact.h"

enum { BFM_FOREST_BOULDERS = 5 };
typedef struct {
    BfmForestTransform root;
    int16_t at[3];
    uint16_t yaw;
    int active;
} BfmForestBoulder;
typedef struct {
    BfmForestBoulder placed[BFM_FOREST_BOULDERS];
    BfmForestGuardBox box;
    int32_t forward_speed;
    BfmForestBarrierArt *art;
} BfmForestBoulders;

int bfm_forest_boulders_load(BfmForestBoulders *rocks, const BfmForestField *field,
                             uint32_t placement_list);
void bfm_forest_boulders_tick(BfmForestBoulders *rocks, int16_t player_z);
unsigned bfm_forest_boulders_draw(const BfmForestBoulders *rocks, BfmForestActor *actors);
int bfm_forest_boulder_contact(const BfmForestBoulders *rocks, const int16_t from[3],
                               const int16_t to[3], int16_t resolved[3]);
int bfm_forest_boulder_obstruction(const BfmForestGuardBox *box, const int16_t at[3],
                                   const int16_t from[3], const int16_t to[3]);
int bfm_forest_boulder_move(BfmForestField *field, unsigned placement,
                            const int16_t from[3], const int16_t to[3]);
int bfm_forest_boulder_path_blocked(BfmForestField *field, unsigned skip,
                                    const int16_t from[3], const int16_t tip[3]);
BfmForestBoulders *bfm_forest_room_boulders(BfmForestField *field);
#endif
