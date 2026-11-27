/* SPDX-License-Identifier: MIT */
#include "village_chimney.h"
#include "forest_dust.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>

enum { WIND_PERIOD = 80, LIFETIME = 90, PATTERNS = 7, BURST_SLOTS = 4,
       /* A puff can overlap births from at most three wind periods. */
       PUFFS = 3 * BURST_SLOTS };

typedef struct {
    int wind_x, wind_z;
    unsigned remaining, cell;
} Puff;

struct BfmVillageChimney {
    int16_t origin[3];
    const uint8_t *patterns;
    BfmForestDust sprite;
    Puff puffs[PUFFS];
    unsigned countdown, pattern, direction, strength;
};

static int floor_fixed(int64_t value, int unit)
{
    return (int)(value / unit - (value % unit < 0));
}

BfmVillageChimney *bfm_village_chimney_open(const MusashiDiscMedia *disc,
                                            const uint8_t *scene, size_t bytes)
{
    const size_t anchor = UINT32_C(0x80191904) - UINT32_C(0x80128158);
    if (!scene || bytes < anchor + 8 + PATTERNS * BURST_SLOTS) return NULL;
    BfmVillageChimney *chimney = calloc(1, sizeof *chimney);
    if (!chimney) return NULL;
    if (!bfm_forest_dust_load(&chimney->sprite, disc)) {
        free(chimney);
        return NULL;
    }
    for (unsigned i = 0; i < 3; ++i) chimney->origin[i] = s16le(scene + anchor + 2*i);
    chimney->patterns = scene + anchor + 8;
    chimney->strength = 128;
    return chimney;
}

void bfm_village_chimney_free(BfmVillageChimney *chimney)
{
    free(chimney);
}

static void emit(BfmVillageChimney *chimney)
{
    const double angle = chimney->direction * (6.28318530717958647693 / 256);
    const int strength = (int)chimney->strength * 64;
    for (unsigned i = 0; i < PUFFS; ++i) {
        if (chimney->puffs[i].remaining) continue;
        chimney->puffs[i] = (Puff){
            .remaining = LIFETIME,
            .wind_x = floor_fixed(lround(4096 * sin(angle)) * strength, 4096),
            .wind_z = floor_fixed(lround(4096 * cos(angle)) * strength, 4096)
        };
        break;
    }
}

void bfm_village_chimney_step(BfmVillageChimney *chimney, BfmGameplayRun *run, unsigned hour)
{
    for (unsigned i = 0; i < PUFFS; ++i) {
        Puff *puff = &chimney->puffs[i];
        if (!puff->remaining) continue;
        if (puff->remaining % 8 == 0 && puff->cell < BFM_FOREST_DUST_CELLS-1) ++puff->cell;
        --puff->remaining;
    }
    if (hour < 7 || hour >= 19 || !run) return;
    const uint8_t *burst = chimney->patterns + BURST_SLOTS * chimney->pattern;
    for (unsigned i = 0; i < BURST_SLOTS; ++i) {
        if (burst[i] == chimney->countdown) {
            emit(chimney);
            break;
        }
    }
    if (chimney->countdown && --chimney->countdown) return;
    chimney->countdown = WIND_PERIOD;
    chimney->direction = (chimney->direction + (bfm_gameplay_random(run) & 15u) - 7u) & 255u;
    chimney->strength = (chimney->strength + (bfm_gameplay_random(run) & 15u) - 7u) & 255u;
    chimney->pattern = bfm_gameplay_random(run) % PATTERNS;
}

static int project(const BfmVillageChimney *chimney, const Puff *puff, const BfmBedroomView *view,
                   BfmForestDustCard *card)
{
    const int age = LIFETIME - (int)puff->remaining;
    /* Horizontal wind is acceleration. With zero initial velocity the nth
     * position integrates n wind increments, then n-1, down to one. */
    const int steps = age * (age + 1) / 2;
    int world[3] = {chimney->origin[0] + floor_fixed(steps * (int64_t)puff->wind_x, 65536),
                    chimney->origin[1] - 8*age,
                    chimney->origin[2] + floor_fixed(steps * (int64_t)puff->wind_z, 65536)};
    int16_t xyz[3];
    for (unsigned i = 0; i < 3; ++i) {
        if (world[i] < INT16_MIN || world[i] > INT16_MAX) return 0;
        xyz[i] = (int16_t)world[i];
    }
    int center[3];
    if (!bfm_bedroom_project(view, xyz, center) || center[2] >= 16380) return 0;
    const BfmForestDust *sprite = &chimney->sprite;
    int reciprocal = (int)(((int64_t)view->focal * 65536 + center[2]/2) / center[2]);
    if (reciprocal > 131071) reciprocal = 131071;
    int scale = 3 * reciprocal / 16;
    int x = center[0] + floor_fixed((int64_t)sprite->left * scale, 4096);
    int y = center[1] + floor_fixed((int64_t)sprite->top * scale, 4096);
    unsigned cell = puff->cell;
    card->bucket = (unsigned)center[2] / 4 + 1;
    card->material = (BfmForestRasterMaterial){.count = 4, .textured = 1, .translucent = 1,
        .page = sprite->page[cell], .clut = 412 * 64 + 352 / 16};
    for (unsigned i = 0; i < 4; ++i) {
        int right = i & 1, bottom = i >> 1;
        card->vertices[i] = (BfmForestRasterVertex){
            .x = x + right * (sprite->width * scale / 4096),
            .y = y + bottom * (sprite->height * scale / 4096),
            .u = (uint8_t)(sprite->u[cell] + right * (sprite->width - 1)),
            .v = (uint8_t)(sprite->v[cell] + bottom * (sprite->height - 1)),
            .rgb = 0x808080
        };
    }
    return 1;
}

int bfm_village_chimney_add(const BfmVillageChimney *chimney, BfmBedroomFigure *figure,
                            const BfmBedroomView *view)
{
    BfmForestDustCard cards[PUFFS];
    unsigned count = 0;
    for (unsigned i = 0; i < PUFFS; ++i) {
        if (chimney->puffs[i].remaining && project(chimney, &chimney->puffs[i], view, &cards[count])) {
            cards[count].slot = i;
            ++count;
        }
    }
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}
