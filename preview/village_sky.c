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

typedef struct {
    int top, height;
    const uint8_t *from, *to;
} SkyBand;

static void draw_bands(const SkyBand *bands, int count, uint32_t pixels[240][320])
{
    static const int dither[4][4] = {
        {-4, 0, -3, 1}, {2, -2, 3, -1},
        {-3, 1, -4, 0}, {3, -1, 2, -2}
    };
    for (int y = 0; y < 240; ++y) {
        uint32_t ink[4] = {0xff000000u, 0xff000000u, 0xff000000u, 0xff000000u};
        for (int band = 0; y >= 10 && y < 230 && band < count; ++band) {
            int offset = y - bands[band].top, span = bands[band].height;
            if (offset < 0 || offset >= span) continue;
            for (int channel = 0; channel < 3; ++channel) {
                int value = (bands[band].from[channel] * (span - offset) +
                             bands[band].to[channel] * offset + span / 2) / span;
                for (int phase = 0; phase < 4; ++phase)
                    ink[phase] |= scanout_channel(value + dither[y & 3][phase]) << (16 - 8*channel);
            }
            break;
        }
        for (int x = 0; x < 320; ++x) pixels[y][x] = ink[x & 3];
    }
}

int bfm_village_sky_draw_lit(const uint8_t *room, size_t bytes, const uint8_t gain[3],
                             uint32_t pixels[240][320])
{
    SkyBand bands[2];
    uint8_t colors[2][6];
    if (!room || bytes < 0x4c || !gain || !pixels) return 0;
    for (int i = 0; i < 2; ++i) {
        bands[i].top = 120 + signed_half(room + 0x38 + 2*i);
        bands[i].height = signed_half(room + 0x3c + 2*i);
        if (bands[i].height <= 0) return 0;
        for (unsigned k = 0; k < 6; ++k)
            colors[i][k] = (uint8_t)(room[0x40 + 6*i + k] * gain[k % 3] / 16);
        bands[i].from = colors[i];
        bands[i].to = bands[i].from + 3;
    }
    draw_bands(bands, 2, pixels);
    return 1;
}

int bfm_village_sky_draw(const uint8_t *room, size_t bytes, uint32_t pixels[240][320])
{
    const uint8_t daylight[3] = {16,16,16};
    return bfm_village_sky_draw_lit(room, bytes, daylight, pixels);
}

int bfm_lower_village_sky_draw(const uint8_t *scene, size_t bytes, unsigned hour,
                               uint32_t pixels[240][320])
{
    const size_t table = 0x8018de74u - 0x80128158u;
    if (!scene || !pixels || hour >= 24 || bytes < table + 5*12) return 0;
    unsigned phase;
    if (hour < 5 || hour >= 22) phase = 4;
    else if (hour < 7) phase = 0;
    else if (hour < 16) phase = 1;
    else if (hour < 19) phase = 2;
    else phase = 3;
    const uint8_t *row = scene + table + phase*12;
    const uint8_t gain[3] = {row[0], row[4], row[8]};
    return bfm_lower_street_sky_draw_gain(gain, pixels);
}

int bfm_lower_street_sky_draw_gain(const uint8_t gain[3], uint32_t pixels[240][320])
{
    if (!gain || !pixels) return 0;
    unsigned r = gain[0], g = gain[1], b = gain[2];
    if (r > 16 || g > 16 || b > 16) return 0;

    /* The outdoor backdrop has four color stops scaled by environmental
     * light levels for the current time of day. */
    const uint8_t stops[4][3] = {
        {5*r/8, 8*g, 255*b/16},
        {143*r/16, 25*g/2, 255*b/16},
        {255*r/16, 255*g/16, 255*b/16},
        {143*r/16, 255*g/16, 45*b/4}
    };
    const SkyBand bands[3] = {
        {4,   140, stops[0], stops[1]},
        {144,  60, stops[1], stops[2]},
        {204,  40, stops[2], stops[3]}
    };
    draw_bands(bands, 3, pixels);
    return 1;
}
