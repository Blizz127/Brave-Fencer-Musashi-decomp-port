/* SPDX-License-Identifier: MIT */
#include "woods_lardwick_motes.h"
#include "woods_fusion_art.h"

enum { DRAW_FIELDS = 10 };

static int16_t world_coordinate(int value)
{
    return (int)(((unsigned)value & 65535u) ^ 32768u) - 32768;
}

void bfm_woods_lardwick_motes_step(BfmWoodsLardwickMotes *motes, BfmGameplayRun *run,
                                   const int16_t root[3], unsigned beam_age)
{
    if (!motes || !run || !root) return;
    unsigned births = beam_age >= 47 && beam_age <= 78 ? 3 : 0;
    for (unsigned i = 0; i < BFM_WOODS_LARDWICK_MOTE_SLOTS; ++i) {
        BfmWoodsLardwickMote *mote = &motes->slots[i];
        /* The emitter allocates before the particles advance. A finishing
         * slot becomes available to the emitter on its following update. */
        if (mote->age) {
            mote->age = mote->age == DRAW_FIELDS + 1 ? 0 : mote->age + 1;
        } else if (births) {
            --births;
            unsigned sample = bfm_gameplay_random(run);
            mote->origin[0] = world_coordinate(root[0] + (int)(sample & 63) - 32);
            mote->origin[1] = root[1];
            mote->origin[2] = world_coordinate(root[2] + (int)((sample >> 6) & 63) - 32);
            mote->color = (sample >> 12) & 3;
            mote->age = 1;
        }
    }
}

int bfm_woods_lardwick_motes_add(const BfmWoodsLardwickMotes *motes, BfmBedroomFigure *figure,
                                 const BfmBedroomView *view)
{
    BfmForestCamera camera;
    if (!motes || !figure || !bfm_woods_effect_camera(view, &camera)) return 0;
    const uint32_t colors[] = {0x0000ff, 0x00ff00, 0xff0000, 0xffffff};
    BfmForestDustCard cards[BFM_WOODS_LARDWICK_MOTE_SLOTS];
    size_t count = 0;
    for (unsigned i = 0; i < BFM_WOODS_LARDWICK_MOTE_SLOTS; ++i) {
        const BfmWoodsLardwickMote *mote = &motes->slots[i];
        if (mote->age < 2) continue;
        int y = mote->origin[1] - 32 * (mote->age - 1);
        int16_t endpoints[4][3];
        /* The second endpoint supplies the ordering depth. The first is black,
         * leaving the bright tip at the upper end of the rising streak. */
        for (unsigned j = 0; j < 4; ++j) {
            endpoints[j][0] = mote->origin[0];
            endpoints[j][1] = world_coordinate(y + (j ? -32 : 32));
            endpoints[j][2] = mote->origin[2];
        }
        BfmForestDustCard line = {0};
        if (!bfm_forest_project_effect_quad(&camera, endpoints, &line)) continue;
        if (line.bucket > 4095) line.bucket = 4095;
        line.material = (BfmForestRasterMaterial){.count = 2, .translucent = 1, .page = 32};
        line.vertices[1].rgb = colors[mote->color];
        cards[count++] = line;
    }
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}
