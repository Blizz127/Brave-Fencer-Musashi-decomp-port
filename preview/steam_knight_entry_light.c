/* SPDX-License-Identifier: MIT */
#include "steam_knight_entry_light.h"
#include <math.h>

static double lobe(double x, double y, double rx, double ry, double gain)
{
    double r = sqrt(x*x/(rx*rx) + y*y/(ry*ry));
    return fmin(64, fmax(0, gain*(1-r)));
}

/* An optical approximation to the entrance burst: two overlapping lobes
 * across the doorway, feathered along the carpet. They share a neutral,
 * saturating additive response. */
static unsigned level(int x, int height, double phase, double opacity)
{
    double width = 128 + 24*phase;
    double base = lobe(x + 144 - 8*phase, height - 44, width, 96 + 12*phase, 200);
    double top = lobe(x + 156, height - 96 - 20*phase, width, 136 - 16*phase, 132 - 44*phase);
    double edge = fmin(1, height / (10 - 5*phase));
    return (unsigned)lround(8 * fmin(31, (base + top)*edge) * opacity);
}

size_t bfm_steam_entry_light(const BfmForestCamera *camera, unsigned age,
                             BfmForestDustCard cards[BFM_STEAM_ENTRY_LIGHT_CARDS])
{
    if (!camera || !cards || age < 234 || age >= 307) return 0;
    double phase = fmin(1, fmax(0, ((int)age - 250) / 12.0));
    double opacity = age <= 262 ? 1 : (307 - age) / 45.0;
    unsigned intensities[17][17];
    for (unsigned row = 0; row <= 16; ++row) {
        int h = row ? (int)row*16 - 8 : 0;
        for (unsigned col = 0; col <= 16; ++col)
            intensities[row][col] = level(-320 + (int)col*22, h, phase, opacity);
    }
    size_t count = 0;
    for (unsigned row = 0; row < 16; ++row) {
        for (unsigned col = 0; col < 16; ++col) {
            const unsigned intensity[4] = {intensities[row][col], intensities[row][col+1],
                                           intensities[row+1][col], intensities[row+1][col+1]};
            if (!(intensity[0] | intensity[1] | intensity[2] | intensity[3])) continue;
            int16_t corners[4][3];
            BfmForestDustCard card = {.material = {.page = 1u << 5, .count = 4, .translucent = 1}};
            for (unsigned k = 0; k < 4; ++k) {
                unsigned r = row + (k >> 1);
                int height = r ? (int)r*16 - 8 : 0;
                corners[k][0] = (int16_t)(-320 + 22*(int)(col + (k & 1)));
                corners[k][1] = (int16_t)(-2178 - height);
                corners[k][2] = -4592;
                card.vertices[k].rgb = intensity[k] * 0x010101u;
            }
            if (bfm_forest_project_effect_quad(camera, corners, &card)) {
                card.slot = (unsigned)count;
                cards[count++] = card;
            }
        }
    }
    return count;
}
