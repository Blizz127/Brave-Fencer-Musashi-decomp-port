/* SPDX-License-Identifier: MIT */
#include "bedroom_reaction.h"
#include <limits.h>

enum { OFF, STARTING, GROWING, HOLDING, SHRINKING };
static unsigned half(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }

int bfm_bedroom_reaction_start(BfmBedroomReaction *e, const uint8_t *scene, size_t bytes)
{
    enum { descriptor = 0x801837c8u - 0x80128158u };
    if (!e || !scene || bytes < descriptor + 12) return 0;
    return bfm_reaction_start_sprite(e, scene + descriptor, bytes - descriptor);
}

int bfm_reaction_start_sprite(BfmBedroomReaction *e, const uint8_t *s, size_t bytes)
{
    if (!e || !s || bytes < 12) return 0;
    unsigned x = half(s+4), y = half(s+6), w = s[2], h = s[3];
    if (!w || !h || s[0] != 255 || x >= 1024 || y >= 512 ||
        4*(x%64)+w > 256 || y%256+h > 256) return 0;
    *e = (BfmBedroomReaction){ .state=STARTING, .sprite=s };
    return 1;
}

void bfm_bedroom_reaction_step(BfmBedroomReaction *e)
{
    if (!e) return;
    switch (e->state) {
    case STARTING:
        e->scale = 512;
        e->lift = 64;
        e->state = GROWING;
        break;
    case GROWING:
        if (e->lift) {
            e->lift -= 16;
            e->scale += 2048;
        } else {
            e->scale = 6144;
            e->remaining = 12;
            e->state = HOLDING;
        }
        break;
    case HOLDING:
        if (!--e->remaining) e->state = SHRINKING;
        break;
    case SHRINKING:
        e->scale -= 2048;
        if (!e->scale) e->state = OFF;
        break;
    }
}

static int project(const BfmBedroomReaction *e, const BfmBedroomView *view,
                   const int16_t player[3], int out[3])
{
    if (!e || !e->scale || !view || !player) return 0;
    int y = player[1] - 128 + e->lift;
    if (y < INT16_MIN || y > INT16_MAX) return 0;
    int16_t center[3] = {player[0],(int16_t)y,player[2]};
    return bfm_bedroom_project(view, center, out);
}

int bfm_bedroom_reaction_depth(const BfmBedroomReaction *e, const BfmBedroomView *view,
                               const int16_t player[3])
{
    int center[3];
    if (!project(e,view,player,center)) return INT_MIN;
    return center[2]/4+1;
}

static int floor_div(int64_t value, int denominator)
{
    return (int)(value >= 0 ? value/denominator : -((-value+denominator-1)/denominator));
}

static int projected_length(int offset, int scale, int reciprocal)
{
    int local = floor_div((int64_t)offset*scale, 4096);
    return floor_div((int64_t)local*reciprocal, 65536);
}

void bfm_bedroom_reaction_draw(const BfmBedroomReaction *e, const BfmBedroomView *view,
                               const int16_t player[3], const uint16_t tiles[512][1024],
                               uint32_t *pixels)
{
    int center[3];
    if (!tiles || !pixels || !project(e,view,player,center)) return;
    const uint8_t *s = e->sprite;
    unsigned tx=half(s+4), ty=half(s+6), palette=s[1];
    int64_t ratio=((int64_t)view->focal*65536+center[2]/2)/center[2];
    int reciprocal = ratio > 131071 ? 131071 : (int)ratio;
    int left=center[0]+projected_length((int16_t)half(s+8), e->scale,reciprocal);
    int top=center[1]+projected_length((int16_t)half(s+10), e->scale,reciprocal);
    int right=left+projected_length(s[2], e->scale,reciprocal);
    int bottom=top+projected_length(s[3], e->scale,reciprocal);
    if (right <= left || bottom <= top) return;
    int x0=left < 0 ? 0 : left, y0=top < 10 ? 10 : top;
    int x1=right > 320 ? 320 : right, y1=bottom > 230 ? 230 : bottom;
    int width=right-left, height=bottom-top;
    unsigned palette_x = palette < 224 ? 352 : 256;
    /* The billboard strip uses its full UV extent. Its two axis-aligned
     * affine gradients round exact half texels to the lower index. */
    for (int y=y0; y<y1; ++y) {
        unsigned sy = (unsigned)(((int64_t)2*(y-top)*s[3]+height-1)/(2*height));
        const uint16_t *row = tiles[(ty+sy)&511];
        for (int x=x0; x<x1; ++x) {
            unsigned sx = (unsigned)(((int64_t)2*(x-left)*s[2]+width-1)/(2*width));
            unsigned entry = row[(tx+sx/4)&1023] >> (4*(sx%4)) & 15u;
            unsigned ink = tiles[256+palette][palette_x+entry];
            if (ink) {
                unsigned red=(ink&31)*255/31, green=((ink>>5)&31)*255/31;
                unsigned blue=((ink>>10)&31)*255/31;
                pixels[320*y+x]=UINT32_C(0xff000000) | red<<16 | green<<8 | blue;
            }
        }
    }
}
