/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_RESIDENTS_H
#define BFM_TOWER_SUMMIT_RESIDENTS_H
#include "forest_field.h"

/* Each resident's rendered body, contact shape and switch position share one actor. */
enum { BFM_SUMMIT_RESIDENTS = 6 };
typedef struct BfmTowerSummitResidents BfmTowerSummitResidents;
typedef struct {
    int16_t position[3];
    uint16_t state, health, reward, stun_ticks, variant;
    uint32_t generation, profile;
} BfmTowerSummitResident;

BfmTowerSummitResidents *bfm_tower_summit_residents_open(const BfmForestField *room);
void bfm_tower_summit_residents_free(BfmTowerSummitResidents *pool);
int bfm_tower_summit_residents_tick(BfmTowerSummitResidents *pool, const int16_t player[3],
                                    int player_scripted, int pickup_started, BfmGameplayRun *run);
/* Returns zero for an unoccupied pool slot. */
int bfm_tower_summit_residents_get(const BfmTowerSummitResidents *pool, unsigned slot,
                                   BfmTowerSummitResident *out);
size_t bfm_tower_summit_residents_actors(const BfmTowerSummitResidents *pool,
                                        BfmForestActor *out, size_t capacity);
/* A segment must contact this resident's mounted body before an effector can use it. */
int bfm_tower_summit_residents_contact(const BfmTowerSummitResidents *pool, unsigned slot,
                                      const int16_t from[3], const int16_t to[3], int16_t center[3], int16_t point[3]);
/* The caller resolves the attack's contact and supplies the winning slot. */
int bfm_tower_summit_residents_stun(BfmTowerSummitResidents *pool, unsigned slot, uint32_t generation);
int bfm_tower_summit_residents_bind(BfmTowerSummitResidents *pool, unsigned slot, uint32_t generation);
int bfm_tower_summit_residents_hold(BfmTowerSummitResidents *pool, unsigned slot,
                                    uint32_t generation, unsigned meter);
int bfm_tower_summit_residents_release(BfmTowerSummitResidents *pool, unsigned slot, uint32_t generation,
                                       BfmGameplayRun *run);
int bfm_tower_summit_residents_consume(BfmTowerSummitResidents *pool, unsigned slot, uint32_t generation,
                                       unsigned *reward);
#endif
