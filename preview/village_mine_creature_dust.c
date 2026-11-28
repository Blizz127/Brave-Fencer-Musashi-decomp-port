/* SPDX-License-Identifier: MIT */
#include "village_mine_creature_dust.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int16_t xyz[3];
    uint8_t occupied, cel;
} CreaturePuff;

struct BfmMineCreatureDust {
    BfmForestDust atlas;
    CreaturePuff puffs[BFM_FOREST_ACTOR_LIMIT];
};

BfmMineCreatureDust *bfm_mine_creature_dust_open(const MusashiDiscMedia *disc)
{
    BfmMineCreatureDust *dust = calloc(1, sizeof *dust);
    if (!dust || !bfm_forest_dust_load(&dust->atlas, disc)) {
        free(dust);
        return NULL;
    }
    return dust;
}

void bfm_mine_creature_dust_free(BfmMineCreatureDust *dust)
{
    free(dust);
}

int bfm_mine_creature_dust_spawn_at(BfmMineCreatureDust *dust, unsigned slot,
                                    const int16_t origin[3])
{
    if (!dust || slot >= BFM_FOREST_ACTOR_LIMIT || !origin || dust->puffs[slot].occupied)
        return 0;
    CreaturePuff *puff = &dust->puffs[slot];
    memcpy(puff->xyz, origin, sizeof puff->xyz);
    puff->cel = 0;
    puff->occupied = 1;
    return 1;
}

int bfm_mine_creature_dust_occupied(const BfmMineCreatureDust *dust, unsigned slot)
{
    return dust && slot < BFM_FOREST_ACTOR_LIMIT && dust->puffs[slot].occupied;
}

void bfm_mine_creature_dust_step_at(BfmMineCreatureDust *dust, unsigned slot)
{
    if (!bfm_mine_creature_dust_occupied(dust, slot)) return;
    CreaturePuff *puff = &dust->puffs[slot];
    if (++puff->cel == BFM_FOREST_DUST_CELLS) puff->occupied = 0;
}

int bfm_mine_creature_dust_draw_at(const BfmMineCreatureDust *dust, unsigned slot,
                                   BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!dust || !figure || !view || view->focal <= 0 || view->focal > UINT16_MAX ||
        slot >= BFM_FOREST_ACTOR_LIMIT) return 0;
    const CreaturePuff *puff = &dust->puffs[slot];
    if (!puff->occupied) return 1;

    /* The sorter projects the body origin, the scaled hotspot and the scaled
     * rectangle size independently at the same depth. */
    const BfmForestDust *atlas = &dust->atlas;
    int16_t points[4][3] = {{0}, {3 * atlas->left, 3 * atlas->top, 0},
                                  {3 * atlas->width, 3 * atlas->height, 0}, {0}};
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t dot = 0;
        for (unsigned k = 0; k < 3; ++k) dot += (int64_t)view->axes[axis][k] * puff->xyz[k];
        int64_t value = view->translation[axis] + dot / 4096 - (dot % 4096 < 0);
        if (value < INT16_MIN || value > INT16_MAX) return 1;
        points[0][axis] = (int16_t)value;
    }
    for (unsigned i = 1; i < 4; ++i) points[i][2] = points[0][2];
    BfmForestCamera plane = {.projection = (uint16_t)view->focal,
                             .rotation = {{4096,0,0}, {0,4096,0}, {0,0,4096}}};
    BfmForestDustCard card;
    if (!bfm_forest_project_effect_quad(&plane, points, &card) || ++card.bucket >= 4096)
        return 1;

    int left = card.vertices[0].x + card.vertices[1].x - card.vertices[3].x;
    int top = card.vertices[0].y + card.vertices[1].y - card.vertices[3].y;
    int width = card.vertices[2].x - card.vertices[3].x;
    int height = card.vertices[2].y - card.vertices[3].y;
    card.slot = slot;
    card.material = (BfmForestRasterMaterial){.count = 4, .textured = 1, .translucent = 1,
        .page = atlas->page[puff->cel], .clut = (412u << 6) | 352u / 16};
    for (unsigned i = 0; i < 4; ++i) {
        unsigned right = i & 1, bottom = i >> 1;
        card.vertices[i] = (BfmForestRasterVertex){
            .x = left + (int)right * width, .y = top + (int)bottom * height,
            .u = (uint8_t)(atlas->u[puff->cel] + right * (atlas->width - 1)),
            .v = (uint8_t)(atlas->v[puff->cel] + bottom * (atlas->height - 1)),
            .rgb = 0x808080};
    }
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, &card, 1);
}
