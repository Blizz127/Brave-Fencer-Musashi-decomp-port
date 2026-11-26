/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_EFFECTS_H
#define BFM_STEAM_KNIGHT_EFFECTS_H
#include "forest_blade.h"
#include "forest_sweep.h"
#include "steam_knight_flames.h"
#include "tower_summit_pickup.h"

enum { BFM_STEAM_WEAPON_CARDS = BFM_FOREST_SWEEP_CARDS + BFM_SUMMIT_PICKUP_GLOW_CARDS };
typedef struct {
    BfmForestBlade fusion;
    int8_t lumina_rim[48];
    BfmForestSweep trail;
    BfmForestSweepEdge next_edge;
    int has_edge;
} BfmSteamWeaponEffects;

int bfm_steam_weapons_load(BfmSteamWeaponEffects *effects, const uint8_t *scene, size_t size);
int bfm_steam_weapons_tick(BfmSteamWeaponEffects *effects, const BfmSteamFlameRay *lumina);
size_t bfm_steam_weapons_project(const BfmSteamWeaponEffects *effects, const BfmForestCamera *camera,
    const BfmForestTransform *root, uint32_t visible, const int8_t fusion[2][3],
    const int8_t lumina[2][3], unsigned tick, BfmForestBladeFrame *blade, int *has_blade,
    BfmForestDustCard out[BFM_STEAM_WEAPON_CARDS]);
#endif
