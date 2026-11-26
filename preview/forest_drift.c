/* SPDX-License-Identifier: MIT */
#include "forest_drift.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

#include <string.h>

int bfm_forest_drift_load(BfmForestDrift *drift, const BfmForestField *field,
                          uint32_t placements)
{
    /* The second permanent room placement is the screen-space emitter.
     * This forest uses its five-sprite entrance variant. */
    const uint8_t *list = bfm_forest_room_bytes(field, placements, 40);
    const uint8_t *heights = bfm_forest_room_bytes(field, UINT32_C(0x801b1568), 32);
    const uint8_t *sprite = bfm_forest_room_bytes(field, UINT32_C(0x801b1590), 12);
    if (!drift || !list || u16le(list + 26) != 33 || u16le(list + 28) != 1 ||
        !heights || !sprite || !sprite[2] || !sprite[3]) return 0;
    unsigned count = 0;
    while (count < 16 && s16le(heights + 2 * count)) ++count;
    if (!count || count == 16) return 0;
    *drift = (BfmForestDrift){.heights = heights, .height_count = count, .sprite = sprite};
    return 1;
}

static void emit(BfmForestDrift *drift, BfmGameplayRun *run, int x)
{
    unsigned scale = 6144 + 1024 * (bfm_gameplay_random(run) & 3);
    int16_t y = s16le(drift->heights + 2 * drift->next_height);
    drift->next_height = (drift->next_height + 1) % drift->height_count;
    BfmForestDriftSprite *sprites = drift->frames[drift->current];
    for (unsigned i = 0; i < BFM_FOREST_DRIFT_CARDS; ++i) {
        if (sprites[i].scale) continue;
        sprites[i] = (BfmForestDriftSprite){x * 65536, y, (uint16_t)scale};
        return;
    }
}

void bfm_forest_drift_start(BfmForestDrift *drift, BfmGameplayRun *run)
{
    if (!run || !drift->sprite || drift->timer) return;
    for (int x = -128; x <= 192; x += 80) emit(drift, run, x);
    drift->timer = 1;
}

void bfm_forest_drift_tick(BfmForestDrift *drift, BfmGameplayRun *run)
{
    if (!run || !drift->timer) return;
    unsigned previous = drift->current;
    drift->current = (previous + 1) % 3;
    BfmForestDriftSprite *sprites = drift->frames[drift->current];
    memcpy(sprites, drift->frames[previous], sizeof drift->frames[previous]);
    /* Entrance births precede the first room effect pass. They acquire
     * their drawables together with the next emitter birth before moving. */
    for (unsigned i = 0; drift->submitted && i < BFM_FOREST_DRIFT_CARDS; ++i) {
        if (!sprites[i].scale) continue;
        sprites[i].x -= (sprites[i].scale - 4096) * 32;
        if (sprites[i].x < -272 * 65536) sprites[i].scale = 0;
    }
    if (--drift->timer == 0) {
        emit(drift, run, 272);
        drift->timer = 30;
    }
    drift->submitted |= 1u << drift->current;
}

unsigned bfm_forest_drift_count(const BfmForestDrift *drift)
{
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_FOREST_DRIFT_CARDS; ++i)
        if (drift->frames[drift->current][i].scale) ++count;
    return count;
}

static int floor_div(int value, int divisor)
{
    return value / divisor - (value % divisor < 0);
}

unsigned bfm_forest_drift_project(const BfmForestDrift *drift,
                                  BfmForestDustCard out[BFM_FOREST_DRIFT_CARDS])
{
    const uint8_t *s = drift->sprite;
    if (!s || !drift->timer) return 0;
    /* Screen coordinates bypass the field camera. The display reads the
     * completed draw buffer two field updates after logical submission. */
    unsigned display = (drift->current + 1) % 3;
    if (!(drift->submitted & (1u << display))) return 0;
    const BfmForestDriftSprite *sprites = drift->frames[display];
    unsigned u = 4 * (u16le(s + 4) % 64), v = u16le(s + 6) % 256;
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_FOREST_DRIFT_CARDS; ++i) {
        int scale = sprites[i].scale;
        if (!scale) continue;
        int x = 160 + floor_div(sprites[i].x, 65536) + floor_div(s16le(s + 8) * scale, 4096);
        int y = 120 + sprites[i].y + floor_div(s16le(s + 10) * scale, 4096);
        int width = s[2] * scale / 4096, height = s[3] * scale / 4096;
        BfmForestDustCard *card = &out[count++];
        card->slot = i;
        card->bucket = 4;
        card->material = (BfmForestRasterMaterial){
            .page = (640 / 64) | (1u << 4) | (3u << 5), .clut = (414u << 6) | (352 / 16),
            .count = 4, .textured = 1, .translucent = 1
        };
        for (unsigned k = 0; k < 4; ++k) {
            int right = k & 1, bottom = k >> 1;
            card->vertices[k] = (BfmForestRasterVertex){x + right * width, y + bottom * height,
                (uint8_t)(u + right * (s[2] - 1)), (uint8_t)(v + bottom * (s[3] - 1)), 0x808080};
        }
    }
    return count;
}
