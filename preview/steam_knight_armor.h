/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_ARMOR_H
#define BFM_STEAM_KNIGHT_ARMOR_H

#include "forest_guard_contact.h"
#include "forest_fusion_power.h"
#include "steam_knight_machine.h"

enum { BFM_STEAM_ARMOR_PLATES = 4 };

typedef struct {
    BfmForestGuardBox box;
    unsigned joint, flags;
    int hp;
} BfmSteamKnightArmorPlate;

typedef struct {
    BfmSteamKnightArmorPlate plate[BFM_STEAM_ARMOR_PLATES];
    unsigned struck;
} BfmSteamKnightArmor;

typedef struct {
    unsigned variant, joint, attack_class;
    int before, after;
    int16_t center[3], point[3], normal[3];
} BfmSteamKnightArmorHit;

int bfm_steam_knight_armor_load(BfmSteamKnightArmor *armor, const uint8_t *scene, size_t bytes);
int bfm_steam_knight_armor_load_power(const MusashiDiscMedia *disc, const uint8_t *scene,
                                    size_t bytes, BfmForestFusionPower *power);
void bfm_steam_knight_armor_begin_swing(BfmSteamKnightArmor *armor);
/* The action owner supplies active world-space Fusion probes and the live
 * damage roll. An empty window, a miss and repeat contact leave HP alone. */
int bfm_steam_knight_armor_strike(BfmSteamKnightArmor *armor, const BfmSteamKnightMachine *machine,
                                  const int16_t from[3], const int16_t ends[][3], size_t ray_count,
                                  unsigned attack_class, uint16_t damage, BfmSteamKnightArmorHit *hit);

#endif
