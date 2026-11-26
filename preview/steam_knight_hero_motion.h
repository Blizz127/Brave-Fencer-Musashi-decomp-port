/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_HERO_MOTION_H
#define BFM_STEAM_KNIGHT_HERO_MOTION_H

#include "forest_field.h"
#include "forest_manual.h"
#include "forest_swing.h"

typedef struct BfmSteamKnightHeroMotion BfmSteamKnightHeroMotion;
typedef struct {
    int issued, roll_strength;
    unsigned power_class;
    uint16_t type;
    size_t count;
    int16_t from[3], ends[BFM_FOREST_STRIKE_RAYS][3];
} BfmSteamKnightHeroStrike;

typedef struct {
    int16_t feet[3], heading;
    unsigned action, motion, elapsed;
    uint32_t body;
} BfmSteamKnightHeroSample;

/* The handover supplies the actual script feet, facing, and the same walking
 * startup speed as the existing player. This controller owns no RPG state. */
BfmSteamKnightHeroMotion *bfm_steam_hero_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const int16_t feet[3], int16_t heading,
    int32_t initial_walk_velocity);
void bfm_steam_hero_free(BfmSteamKnightHeroMotion *hero);
const BfmSteamKnightHeroSample *bfm_steam_hero_sample(const BfmSteamKnightHeroMotion *hero);
/* One ordinary field update. x and y are the live controller axes (right/down
 * positive). Segments are valid only for this update's active blade window. */
int bfm_steam_hero_tick(BfmSteamKnightHeroMotion *hero, const BfmForestCamera *camera,
    int x, int y, uint16_t pressed, uint16_t held, BfmSteamKnightHeroStrike *strike);
/* An independently established physical impact may interrupt the current
 * action. Heading is the world-space direction of knockback. The caller owns HP. */
int bfm_steam_hero_recoil(BfmSteamKnightHeroMotion *hero, int16_t heading);

#endif
