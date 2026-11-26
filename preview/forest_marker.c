/* SPDX-License-Identifier: MIT */
#include "forest_marker.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>

int bfm_forest_marker_start(BfmForestMarker *marker, const BfmForestField *field,
                            const int16_t owner[3])
{
    const uint8_t *sprite = bfm_forest_room_bytes(field, UINT32_C(0x80187f34), 12);
    if (!marker || !owner || !bfm_reaction_start_sprite(&marker->reaction, sprite, 12))
        return 0;
    for (unsigned i = 0; i < 3; ++i) marker->owner[i] = owner[i];
    /* Construction and the first growth update precede its first submission. */
    bfm_bedroom_reaction_step(&marker->reaction);
    bfm_bedroom_reaction_step(&marker->reaction);
    return 1;
}

void bfm_forest_marker_advance(BfmForestMarker *marker, const int16_t owner[3])
{
    if (!marker || !owner || !marker->reaction.scale) return;
    for (unsigned i = 0; i < 3; ++i) marker->owner[i] = owner[i];
    bfm_bedroom_reaction_step(&marker->reaction);
}

static int down(int64_t n, int divisor)
{
    return (int)(n / divisor - (n % divisor < 0));
}

int bfm_forest_marker_project(const BfmForestMarker *marker, const BfmForestCamera *camera,
                              BfmForestDustCard *card)
{
    if (!marker || !camera || !card || !camera->projection ||
        !marker->reaction.sprite || marker->reaction.scale <= 0) return 0;
    const uint8_t *s = marker->reaction.sprite;
    int32_t world[3] = {marker->owner[0], marker->owner[1] - 120 + marker->reaction.lift,
                        marker->owner[2]};
    int64_t view[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        const int16_t *r = camera->rotation[axis];
        int64_t dot = (int64_t)r[0] * world[0] + (int64_t)r[1] * world[1] + (int64_t)r[2] * world[2];
        view[axis] = (int64_t)down(dot, 4096) + camera->translation[axis];
    }
    if (view[2] <= 0 || view[2] / 4 + 1 >= 4096 || view[0] < INT16_MIN ||
        view[0] > INT16_MAX || view[1] < INT16_MIN || view[1] > INT16_MAX)
        return 0;
    int64_t q = ((int64_t)camera->projection * 65536 + view[2] / 2) / view[2];
    if (q > 131071) q = 131071;
    int scale = (int)(marker->reaction.scale * q / 65536);
    int x = 160 + down(view[0] * q, 65536) + down((int64_t)s16le(s + 8) * scale, 4096);
    int y = 120 + down(view[1] * q, 65536) + down((int64_t)s16le(s + 10) * scale, 4096);
    int w = s[2] * scale / 4096, h = s[3] * scale / 4096;
    if (!w || !h) return 0;
    unsigned tx = u16le(s + 4), ty = u16le(s + 6);
    unsigned u = 4 * (tx % 64), v = ty % 256, palette_x = s[1] < 224 ? 352 : 256;
    card->slot = 0;
    card->bucket = (unsigned)(view[2] / 4 + 1);
    card->material = (BfmForestRasterMaterial){
        .page = (uint16_t)(tx / 64 | (ty / 256) << 4 | 1u << 5),
        .clut = (uint16_t)((256u + s[1]) << 6 | palette_x / 16), .count = 4, .textured = 1
    };
    for (unsigned i = 0; i < 4; ++i) {
        int right = i & 1, bottom = i >> 1;
        card->vertices[i] = (BfmForestRasterVertex){x + right * w, y + bottom * h,
            (uint8_t)(u + right * s[2]), (uint8_t)(v + bottom * s[3]), 0x808080};
    }
    return 1;
}
