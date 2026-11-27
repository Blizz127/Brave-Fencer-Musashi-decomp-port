/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_FOOT_DUST_H
#define BFM_WOODS_FOOT_DUST_H

#include "steam_knight_charge_effect.h"

enum { BFM_WOODS_FOOT_DUST = 2 };

typedef struct {
    BfmForestDust atlas;
    BfmForestDustParticle particles[BFM_STEAM_CHARGE_SLOTS];
} BfmWoodsFootDust;

int bfm_woods_foot_dust_load(BfmWoodsFootDust *dust, const MusashiDiscMedia *disc);
/* Emit during Musashi's movement, before the ascending field effects pass. */
void bfm_woods_foot_dust_emit(BfmWoodsFootDust *dust, BfmSteamChargeEffects *pool,
    BfmForestDustEmission event, const int16_t feet[3], const int32_t eye[3], int yaw);
void bfm_woods_foot_dust_visit(BfmWoodsFootDust *dust, BfmSteamChargeEffects *pool, unsigned slot);
unsigned bfm_woods_foot_dust_project(const BfmWoodsFootDust *dust, const BfmSteamChargeEffects *pool,
    const BfmForestCamera *camera, BfmForestDustCard *cards, unsigned capacity);

#endif
