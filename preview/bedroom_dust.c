/* SPDX-License-Identifier: MIT */
#include "bedroom_dust.h"
#include "forest_raster.h"
#include <limits.h>
#include <math.h>

void bfm_bedroom_dust_step(BfmBedroomDust *dust, const int16_t point[3], unsigned yaw,
                           const int eye[3], int emit)
{
    for (unsigned i=0; i<3; ++i) {
        BfmBedroomPuff *p = &dust->puffs[i];
        if (p->age && ++p->age == BFM_HERO_SMOKE_CELLS) p->age = 0;
        if (!p->age && emit) {
            int direction = emit;
            emit = 0;
            p->age = 1;
            double angle = (yaw & 4095u) * (6.2831853071795864769/4096);
            p->velocity[0] = lround(sin(angle)*4096) * (256*direction);
            p->velocity[2] = lround(cos(angle)*4096) * (256*direction);
            p->velocity[1] = -65536;
            int delta[3] = {eye[0]-point[0], eye[1]-point[1], eye[2]-point[2]};
            double length = hypot(hypot(delta[0],delta[1]),delta[2]);
            for (unsigned axis=0; axis<3; ++axis) {
                int shift = length ? ((int)(4096*delta[axis]/length) >> 6) : 0;
                p->position[axis] = (int64_t)(point[axis]+shift)*65536;
            }
        }
        if (p->age) {
            for (unsigned axis=0; axis<3; ++axis) {
                p->position[axis] += p->velocity[axis];
                if (axis != 1) p->velocity[axis] = (p->velocity[axis] >> 1) + (p->velocity[axis] >> 2);
            }
        }
    }
}

static int center(const BfmBedroomPuff *p, const BfmBedroomView *view, int out[3])
{
    int16_t xyz[3];
    if (!p->age) return 0;
    for (unsigned i=0;i<3;++i) xyz[i] = (int16_t)(p->position[i] >> 16);
    return bfm_bedroom_project(view,xyz,out);
}

int bfm_bedroom_puff_depth(const BfmBedroomPuff *puff, const BfmBedroomView *view)
{
    int p[3];
    return center(puff,view,p) ? p[2]/4+2 : INT_MIN;
}

void bfm_bedroom_puff_draw(const BfmBedroomDust *dust, unsigned index, const BfmBedroomView *view,
                           const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    int p[3];
    const BfmBedroomPuff *part = &dust->puffs[index];
    if (!center(part,view,p)) return;
    const BfmHeroSmokeMaterial *card = &dust->sprite;
    const BfmHeroSmokeFrame *frame = bfm_hero_smoke_frame(card,0,part->age);
    if (!frame) return;
    int scale = (int)(((int64_t)view->focal*65536+p[2]/2)/p[2]);
    if (scale > 131071) scale = 131071;
    scale = 2*scale/16;
    int left = p[0]+(card->left*scale >> 12), top = p[1]+(card->top*scale >> 12);
    int width = card->width*scale/4096, height=card->height*scale/4096;
    BfmForestRasterVertex corners[4];
    for (unsigned k=0; k<4; ++k)
        corners[k] = (BfmForestRasterVertex){.x=left+(k&1 ? width : 0),.y=top+(k&2 ? height : 0),
            .u=frame->uv[k][0], .v=frame->uv[k][1], .rgb=0x808080};
    BfmForestRaster raster = {.tiles=tiles,.pixels=pixels,.top=10,.bottom=229};
    BfmForestRasterMaterial material = {.page=frame->page,.clut=frame->palette,.count=4,.textured=1,.translucent=1};
    bfm_forest_raster_face(&raster,&material,corners);
}
