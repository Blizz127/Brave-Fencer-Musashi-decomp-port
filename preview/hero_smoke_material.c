/* SPDX-License-Identifier: MIT */
#include "hero_smoke_material.h"

static unsigned word(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static int16_t signed_word(const uint8_t *p)
{
    unsigned raw = word(p);
    int value = raw & 32768u ? (int)raw - 65536 : (int)raw;
    return (int16_t)value;
}

static uint32_t longword(const uint8_t *p)
{
    return (uint32_t)word(p) | (uint32_t)word(p + 2) << 16;
}

static int read_frame(BfmHeroSmokeFrame *frame, const uint8_t *record,
                      unsigned width, unsigned height, unsigned last)
{
    unsigned x = word(record), y = word(record + 2);
    uint32_t timing = longword(record + 4);
    unsigned duration = timing & 63u;
    if (x >= 1024 || y >= 512 || !duration || (timing & ~63u) != (last ? 192u : 0u))
        return 0;

    /* Source X is in VRAM words. Four 4-bit texels share each word. */
    unsigned u = (x % 64) * 4, v = y % 256;
    if (u + width > 256 || v + height > 256)
        return 0;
    frame->page = (uint16_t)(x / 64 | (y / 256) << 4 | 1u << 5);
    /* The reveal uses the scene's neutral additive palette at (352,412).
     * The palette contents and shared texture are already in the tile map. */
    frame->palette = (uint16_t)(412u << 6 | 352u / 16);
    frame->fields = (uint8_t)duration;
    for (unsigned i = 0; i < 4; ++i) {
        frame->uv[i][0] = (uint8_t)(u + ((i & 1) ? width - 1 : 0));
        frame->uv[i][1] = (uint8_t)(v + ((i & 2) ? height - 1 : 0));
    }
    return 1;
}

int bfm_hero_smoke_load(BfmHeroSmokeMaterial *out, const uint8_t *scene, size_t bytes)
{
    const size_t first_track = 0x5ce0c, pointers = 0x5cf00, card = 0x5cf0c;
    const unsigned track_bytes = BFM_HERO_SMOKE_CELLS * 8;
    const size_t positions = 0x60bf0;
    if (!out || !scene || bytes < positions + 8 * BFM_HERO_SMOKE_ORIGINS)
        return 0;

    BfmHeroSmokeMaterial parsed = {0};
    parsed.width = scene[card + 2];
    parsed.height = scene[card + 3];
    if (!parsed.width || !parsed.height)
        return 0;
    parsed.left = signed_word(scene + card + 8);
    parsed.top = signed_word(scene + card + 10);

    /* The three pointers address consecutive tracks in the same mounted
     * scene allocation; only their relative offsets are needed on the host. */
    uint32_t first = longword(scene + pointers);
    for (unsigned t = 0; t < BFM_HERO_SMOKE_TRACKS; ++t) {
        uint32_t ptr = longword(scene + pointers + 4 * t);
        if (ptr < first || ptr - first != t * track_bytes)
            return 0;
        const uint8_t *list = scene + first_track + (ptr - first);
        for (unsigned k = 0; k < BFM_HERO_SMOKE_CELLS; ++k)
            if (!read_frame(&parsed.tracks[t][k], list + k * 8, parsed.width,
                            parsed.height, k == BFM_HERO_SMOKE_CELLS - 1))
                return 0;
    }
    for (unsigned i = 0; i < BFM_HERO_SMOKE_ORIGINS; ++i) {
        const uint8_t *point = scene + positions + 8 * i;
        if (word(point + 6) != 0) return 0;
        for (unsigned axis = 0; axis < 3; ++axis)
            parsed.origin[i][axis] = signed_word(point + 2 * axis);
    }
    *out = parsed;
    return 1;
}

const BfmHeroSmokeFrame *bfm_hero_smoke_frame(const BfmHeroSmokeMaterial *material,
                                              unsigned track, unsigned field_age)
{
    if (!material || track >= BFM_HERO_SMOKE_TRACKS)
        return NULL;
    for (unsigned cell = 0; cell < BFM_HERO_SMOKE_CELLS; ++cell) {
        const BfmHeroSmokeFrame *frame = &material->tracks[track][cell];
        if (field_age < frame->fields)
            return frame;
        field_age -= frame->fields;
    }
    return NULL;
}
