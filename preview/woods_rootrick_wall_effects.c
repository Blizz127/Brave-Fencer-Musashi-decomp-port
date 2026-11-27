/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_wall_effects.h"
#include "../pc_port/forest_player_bytes.h"

enum { CHIP_UPDATES = 32, BORN = CHIP_UPDATES + 2 };

int bfm_woods_rootrick_wall_art_load(BfmWoodsRootrickWallArt *art,
                                     const uint8_t *scene, size_t bytes)
{
    const size_t offset = UINT32_C(0x80194744) - UINT32_C(0x80128158);
    if (!art || !scene || bytes < offset + 12) return 0;
    const uint8_t *sprite = scene + offset;
    if (sprite[0] != 255 || !sprite[2] || sprite[2] > 16 || !sprite[3] || sprite[3] > 16)
        return 0;
    *art = (BfmWoodsRootrickWallArt){.sprite = sprite};
    return 1;
}

static int q12_floor(int64_t product)
{
    return (int)(product / 4096 - (product % 4096 < 0));
}

static void emit(BfmWoodsRootrickWallArt *art, BfmGameplayRun *run, const int16_t origin[3],
                 const BfmForestTransform *parent, unsigned variant)
{
    for (unsigned i = 0; i < BFM_ROOTRICK_WALL_CHIPS; ++i) {
        BfmWoodsWallChip *chip = &art->chips[i];
        if (chip->life) continue;
        unsigned r = bfm_gameplay_random(run);
        unsigned pitch = variant ? 3328 + 4 * (r & 255u) : 576 + (r & 255u);
        unsigned yaw = 3584 + ((r & 0x7f00u) >> 5);
        int speed = variant ? 512 : 4096;
        BfmForestTransform launch;
        bfm_forest_actor_pitch_yaw(pitch, yaw, origin, &launch);
        for (unsigned axis = 0; axis < 3; ++axis) {
            int64_t dot = 0;
            for (unsigned k = 0; k < 3; ++k)
                dot += (int32_t)parent->rotation[axis][k] * launch.rotation[k][2];
            chip->velocity[axis] = q12_floor((int64_t)q12_floor(dot) * speed) * 256;
            chip->position[axis] = (uint32_t)origin[axis] << 16;
        }
        chip->variant = variant;
        chip->life = BORN;
        return;
    }
}

int bfm_woods_rootrick_wall_art_step(BfmWoodsRootrickWallArt *art, BfmGameplayRun *run,
                                     const int16_t hero_site[3], const BfmForestTransform *hero_root,
                                     const int16_t sites[][3], unsigned site_count,
                                     const BfmForestTransform *rival_root)
{
    if (!art || !art->sprite || !run || (hero_site && !hero_root) ||
        site_count > 2 || (site_count && (!sites || !rival_root))) return 0;

    if (hero_site) {
        emit(art, run, hero_site, hero_root, 0);
        emit(art, run, hero_site, hero_root, 1);
    }
    for (unsigned i = 0; i < site_count; ++i) {
        emit(art, run, sites[i], rival_root, 1);
        emit(art, run, sites[i], rival_root, 1);
    }
    for (unsigned i = 0; i < BFM_ROOTRICK_WALL_CHIPS; ++i) {
        BfmWoodsWallChip *chip = &art->chips[i];
        if (!chip->life) continue;
        /* New allocations draw at the actual contact for the birth field. */
        if (--chip->life == BORN - 1 || !chip->life) continue;
        for (unsigned axis = 0; axis < 3; ++axis)
            chip->position[axis] += (uint32_t)chip->velocity[axis];
        chip->velocity[1] += 106496;
    }
    return 1;
}

int bfm_woods_rootrick_wall_art_add(const BfmWoodsRootrickWallArt *art,
                                    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!art || !art->sprite || !figure || !view || view->focal <= 0) return 0;
    const uint8_t *s = art->sprite;
    BfmForestDustCard cards[BFM_ROOTRICK_WALL_CHIPS];
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_ROOTRICK_WALL_CHIPS; ++i) {
        const BfmWoodsWallChip *chip = &art->chips[i];
        if (!chip->life) continue;
        int16_t foot[3];
        for (unsigned axis = 0; axis < 3; ++axis) foot[axis] = (int16_t)(chip->position[axis] >> 16);
        int center[3];
        if (!bfm_bedroom_project(view, foot, center) || center[2] / 4 + 1 >= 4096) continue;
        /* Pivot and extent get separate perspective rounding at the same depth. */
        int scale = chip->variant ? 6144 : 4096;
        BfmBedroomView local = {.axes={{scale,0,0},{0,4096,0},{0,0,4096}}, .focal=view->focal};
        int16_t pivot[3] = {s16le(s+8), s16le(s+10), (int16_t)center[2]};
        int16_t size[3] = {s[2], s[3], pivot[2]};
        int start[3], end[3];
        bfm_bedroom_project(&local, pivot, start);
        bfm_bedroom_project(&local, size, end);
        int x = center[0] + start[0] - 160, y = center[1] + start[1] - 120;
        BfmForestDustCard *card = &cards[count++];
        *card = (BfmForestDustCard){.slot=i, .bucket=(unsigned)center[2]/4 + 1,
              .material={.count=4, .textured=1, .page=0x39, .clut=(256u+42u)*64u+352u/16u}};
        unsigned u = chip->variant ? 16u : 0u;
        for (unsigned k = 0; k < 4; ++k) {
            card->vertices[k] = (BfmForestRasterVertex){
                x + (k & 1 ? end[0]-160 : 0), y + (k & 2 ? end[1]-120 : 0),
                (uint8_t)(u + (k & 1 ? s[2]-1 : 0)), (uint8_t)(240 + (k & 2 ? s[3]-1 : 0)),
                0x808080};
        }
    }
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}
