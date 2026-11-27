/* SPDX-License-Identifier: MIT */
#include "woods_pole_pickup_art.h"
#include "woods_capture_light.h"
#include "woods_fusion_art.h"

enum { LIGHT_RADIUS = 16, LIGHT_FACES = 6 };

int bfm_woods_pole_pickup_art_load(BfmWoodsPolePickupArt *art,
                                   const uint8_t *scene, size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158);
    size_t colors = UINT32_C(0x801e95c0) - base;
    size_t palettes = UINT32_C(0x80193e94) - base;
    if (!art || !scene || bytes < colors + 8 || bytes < palettes + 5) return 0;
    *art = (BfmWoodsPolePickupArt){.colors = scene + colors,
                                   .number_palette = scene[palettes + 4]};
    return 1;
}

void bfm_woods_pole_pickup_art_step(BfmWoodsPolePickupArt *art, const int16_t hero[3],
                                    unsigned amount)
{
    if (!art) return;
    for (unsigned i = 0; i < BFM_WOODS_POLE_PICKUP_COUNT; ++i)
        bfm_forest_damage_digits_advance(&art->numbers[i]);
    if (!amount || amount > 999 || !hero) return;
    for (unsigned i = 0; i < BFM_WOODS_POLE_PICKUP_COUNT; ++i) {
        BfmForestDamageDigits *digits = &art->numbers[i];
        if (digits->amount) continue;
        bfm_forest_damage_digits_start_data(digits, &art->number_palette, hero, (uint16_t)amount);
        digits->bp_credit = 1;
        break;
    }
}

static size_t project_glow(const BfmWoodsPolePickup *item, const BfmBedroomView *view,
                           const BfmForestCamera *camera, uint32_t rim, uint32_t hub,
                           BfmForestDustCard cards[LIGHT_FACES])
{
    int screen[3];
    if (!bfm_bedroom_project(view, item->xyz, screen)) return 0;
    const int32_t center[3] = {item->xyz[0], item->xyz[1], item->xyz[2]};
    int scale = 3072 + 1536 * (int)item->tier;
    size_t count = bfm_woods_radial_light_project(camera, center, LIGHT_RADIUS, scale,
                                                 hub, cards, LIGHT_FACES);
    /* The sprite origin lies sixteen local units above its placement on
     * the screen's vertical axis, at the placement's original depth. */
    int64_t ratio = ((int64_t)view->focal * 65536 + screen[2] / 2) / screen[2];
    if (ratio > 131071) ratio = 131071;
    int offset = -(int)((16 * scale / 4096 * ratio + 65535) / 65536);
    for (size_t i = 0; i < count; ++i) {
        for (unsigned k = 0; k < 4; ++k) {
            cards[i].vertices[k].y += offset;
            if (k != 2) cards[i].vertices[k].rgb = rim;
        }
    }
    return count;
}

int bfm_woods_pole_pickup_art_add(const BfmWoodsPolePickupArt *art,
                                  const BfmWoodsPolePickups *pickups, BfmBedroomFigure *figure,
                                  const BfmBedroomView *view, unsigned field_tick)
{
    BfmForestCamera camera;
    if (!art || !art->colors || !pickups || !figure || !bfm_woods_effect_camera(view, &camera))
        return 0;
    unsigned half = (field_tick & 15u) / 2;
    unsigned pulse = field_tick & 16u ? 9 + half : 16 - half;
    uint32_t rim = 0, hub = 0;
    for (unsigned c = 0; c < 3; ++c) {
        rim |= (uint32_t)art->colors[c] << (8*c);
        hub |= (uint32_t)(pulse * (art->colors[4+c] >> 4)) << (8*c);
    }
    for (unsigned i = 0; i < BFM_WOODS_POLE_PICKUP_COUNT; ++i) {
        if (pickups->items[i].collected) continue;
        BfmForestDustCard glow[LIGHT_FACES];
        size_t count = project_glow(&pickups->items[i], view, &camera, rim, hub, glow);
        if (count && !bfm_bedroom_figure_add_fusion(figure, view, NULL, glow, count)) return 0;
    }
    for (unsigned i = 0; i < BFM_WOODS_POLE_PICKUP_COUNT; ++i) {
        BfmForestDustCard cards[BFM_FOREST_DAMAGE_DIGIT_CARDS];
        unsigned count = bfm_forest_damage_digits_project(&art->numbers[i], &camera, cards);
        if (count && !bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count)) return 0;
    }
    return 1;
}
