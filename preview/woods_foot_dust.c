/* SPDX-License-Identifier: MIT */
#include "woods_foot_dust.h"

int bfm_woods_foot_dust_load(BfmWoodsFootDust *dust, const MusashiDiscMedia *disc)
{
    return dust && bfm_forest_dust_load(&dust->atlas, disc);
}

void bfm_woods_foot_dust_emit(BfmWoodsFootDust *dust, BfmSteamChargeEffects *pool,
    BfmForestDustEmission event, const int16_t feet[3], const int32_t eye[3], int yaw)
{
    BfmForestDustParticle particle;
    if (!dust || !pool || !dust->atlas.width ||
        !bfm_forest_foot_dust_start(&particle,event,feet,eye,yaw)) return;
    int slot = bfm_steam_charge_reserve(pool,BFM_WOODS_FOOT_DUST);
    if (slot >= 0) dust->particles[slot] = particle;
}

void bfm_woods_foot_dust_visit(BfmWoodsFootDust *dust, BfmSteamChargeEffects *pool, unsigned slot)
{
    if (dust && pool && slot < BFM_STEAM_CHARGE_SLOTS &&
        pool->slots[slot].kind == BFM_WOODS_FOOT_DUST &&
        !bfm_forest_foot_dust_advance(&dust->particles[slot]))
        bfm_steam_charge_release(pool,slot,BFM_WOODS_FOOT_DUST);
}

unsigned bfm_woods_foot_dust_project(const BfmWoodsFootDust *dust, const BfmSteamChargeEffects *pool,
    const BfmForestCamera *camera, BfmForestDustCard *cards, unsigned capacity)
{
    if (!dust || !pool || !camera || !cards) return 0;
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_STEAM_CHARGE_SLOTS && count < capacity; ++i) {
        if (pool->slots[i].kind == BFM_WOODS_FOOT_DUST)
            count += bfm_forest_foot_dust_project(&dust->atlas,&dust->particles[i],camera,i,&cards[count]);
    }
    return count;
}
