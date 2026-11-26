/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_FLAMES_H
#define BFM_STEAM_KNIGHT_FLAMES_H

#include "forest_dust.h"
#include "forest_guard_contact.h"

enum { BFM_STEAM_FLAMES = 8 };

typedef struct {
    uint16_t page, clut;
    uint8_t u, v, width, height;
    int16_t left, top;
} BfmSteamFlameSprite;

typedef struct {
    int16_t from[3], to[3];
} BfmSteamFlameRay;

typedef struct {
    BfmSteamFlameSprite burning[16], falling[8];
    BfmForestGuardBox bounds;
    int16_t center[3];
    struct {
        int32_t at[3];
        unsigned fade;
        int lit;
    } children[BFM_STEAM_FLAMES];
    unsigned growth, frame, remaining;
} BfmSteamKnightFlames;

int bfm_steam_knight_flames_load(BfmSteamKnightFlames *flames, const uint8_t *scene, size_t bytes);
void bfm_steam_knight_flames_start(BfmSteamKnightFlames *flames, const int16_t center[3]);
/* Advance once per field actor update. Supply world-space probes only during
 * an active Spin Attack strike window. A missing probe cannot put out a fire. */
void bfm_steam_knight_flames_tick(BfmSteamKnightFlames *flames, const BfmSteamFlameRay *rays,
                                  size_t ray_count);
unsigned bfm_steam_knight_flames_remaining(const BfmSteamKnightFlames *flames);
size_t bfm_steam_knight_flames_project(const BfmSteamKnightFlames *flames,
                                      const BfmForestCamera *camera,
                                      BfmForestDustCard out[BFM_STEAM_FLAMES]);

#endif
