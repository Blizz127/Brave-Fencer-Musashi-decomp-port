/* SPDX-License-Identifier: MIT */
#include "village_sky.h"

static int signed_half(const uint8_t *p)
{
    int value = p[0] | (int)p[1] << 8;
    return value >= 32768 ? value - 65536 : value;
}

static unsigned scanout_channel(int value)
{
    if (value <= 0) return 0;
    if (value >= 255) return 255;
    return (unsigned)(value / 8) * 255 / 31;
}

int bfm_village_sky_draw(const uint8_t *room, size_t bytes,
                        uint32_t pixels[240][320])
{
    static const int dither[4][4] = {
        {-4, 0, -3, 1}, {2, -2, 3, -1},
        {-3, 1, -4, 0}, {3, -1, 2, -2}
    };
    if (!room || bytes < 0x4c || !pixels) return 0;
    int top[2], height[2];
    for (int i = 0; i < 2; ++i) {
        top[i] = 120 + signed_half(room + 0x38 + 2*i);
        height[i] = signed_half(room + 0x3c + 2*i);
        if (height[i] <= 0) return 0;
    }
    for (int y = 0; y < 240; ++y) {
        uint32_t ink[4] = {0xff000000u, 0xff000000u, 0xff000000u, 0xff000000u};
        for (int band = 0; y >= 10 && y < 230 && band < 2; ++band) {
            int offset = y - top[band], span = height[band];
            if (offset < 0 || offset >= span) continue;
            const uint8_t *endpoints = room + 0x40 + 6*band;
            for (int channel = 0; channel < 3; ++channel) {
                int value = (endpoints[channel] * (span - offset) +
                             endpoints[channel + 3] * offset + span / 2) / span;
                for (int phase = 0; phase < 4; ++phase)
                    ink[phase] |= scanout_channel(value + dither[y & 3][phase]) << (16 - 8*channel);
            }
            break;
        }
        for (int x = 0; x < 320; ++x) pixels[y][x] = ink[x & 3];
    }
    return 1;
}
