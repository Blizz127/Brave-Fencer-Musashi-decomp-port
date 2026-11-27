/* SPDX-License-Identifier: MIT */
#include "woods_pole_view.h"
#include "field_waypoint.h"
#include <math.h>

static const uint32_t base = UINT32_C(0x80128158);
static unsigned word(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static uint32_t address(const uint8_t *p)
{
    return word(p) | (uint32_t)word(p+2) << 16;
}

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t addr, size_t need)
{
    if (addr < base || (size_t)(addr-base) > bytes) return NULL;
    size_t offset = addr-base;
    return need <= bytes-offset ? scene+offset : NULL;
}

int bfm_woods_pole_view_load(BfmWoodsPoleView *pole, const uint8_t *scene, size_t bytes)
{
    if (!pole || !scene) return 0;
    const uint8_t *room = mounted(scene,bytes,UINT32_C(0x801961e8),0xb8);
    if (!room) return 0;
    uint32_t cursor = address(room+0xb4);
    const uint8_t *preset = NULL;
    for (;;) {
        const uint8_t *actor = mounted(scene,bytes,cursor,20);
        if (!actor) return 0;
        unsigned type = word(actor+6);
        if (!type) break;
        if (type == 255) {
            if (preset) return 0;
            preset = mounted(scene,bytes,address(actor+16),40);
            if (!preset) return 0;
        }
        if (cursor > UINT32_MAX-20) return 0;
        cursor += 20;
    }
    /* This mount uses a level-roll camera, constrained above on the Y axis. */
    if (!preset || address(preset)==0 || address(preset)>4096 ||
        address(preset+4)==0 || address(preset+4)>8192 ||
        word(preset+8)>4095 || word(preset+10)>4095 || word(preset+12)!=0 ||
        word(preset+30)!=2 || word(preset+38)!=0) return 0;
    BfmWoodsPoleView next = {
        .focal=(int)address(preset), .radius=(int)address(preset+4),
        .pitch=(int)word(preset+8), .yaw=(int)word(preset+10),
        .max_y=(int16_t)word(preset+26)
    };
    for (int i=0; i<3; ++i) next.target_offset[i]=(int16_t)word(preset+16+2*i);
    *pole = next;
    return 1;
}

static int approach(int value, int goal, int divisor)
{
    int difference = goal-value;
    while (divisor>1 && difference/divisor==0) divisor/=2;
    return value+difference/divisor;
}

static int turn(int value, int goal)
{
    int d = ((goal-value+2048)&4095)-2048;
    return approach(value,value+d,4)&4095;
}

void bfm_woods_pole_view_reference(BfmWoodsPoleView *p, const int16_t foot[3],
                                   int entering, int16_t reference[3])
{
    if (!p || !foot || !reference) return;
    if (entering) p->clear_ticks=0;
    int y=foot[1];
    if (p->clear_ticks && y>p->max_y) y=p->max_y;
    p->player_y = entering || !p->has_reference ? y : approach(p->player_y,y,4);
    p->has_reference=1;
    reference[0]=foot[0];
    reference[1]=(int16_t)p->player_y;
    reference[2]=foot[2];
}

int bfm_woods_pole_view_active(const BfmWoodsPoleView *p)
{
    return p && p->clear_ticks != 0;
}

/* The close view uses the game's Q12 angular convention. Its quadrant
 * endpoints repeat. The ordinary mountain camera has a separate Q15 orbit. */
static int sine12(unsigned angle)
{
    unsigned i=angle&1023u;
    if (angle&1024u) i=1023u-i;
    int v=(int)lround(4096.0*sin(i*(6.28318530717958647693/4096.0)));
    return angle&2048u ? -v : v;
}

static int scale12(int a, int b)
{
    int v=a*b;
    return v>=0 ? v/4096 : -((-v+4095)/4096);
}

void bfm_woods_pole_view_step(BfmWoodsPoleView *p, int gripped,
                              const int16_t foot[3], const int16_t ref[3],
                              int ordinary_pitch, int ordinary_offset_y, BfmVillageCamera *camera)
{
    if (!p || !foot || !ref || !camera) return;
    if (!p->clear_ticks) {
        if (gripped) {
            p->saved=*camera;
            p->pitch_now=ordinary_pitch;
            p->yaw_now=0;
            p->offset_now[0]=p->offset_now[2]=0;
            p->offset_now[1]=ordinary_offset_y;
            p->player_y=foot[1]>p->max_y ? p->max_y : foot[1];
            p->clear_ticks=32;
        }
        return;
    }
    if (gripped) p->clear_ticks=32;
    else if (--p->clear_ticks==0) {
        *camera=p->saved;
        return;
    }
    camera->focal=approach(camera->focal,p->focal,4);
    camera->radius=approach(camera->radius,p->radius,4);
    p->pitch_now=turn(p->pitch_now,p->pitch);
    p->yaw_now=turn(p->yaw_now,p->yaw);
    for (int i=0; i<3; ++i) {
        p->offset_now[i]=approach(p->offset_now[i],p->target_offset[i],16);
        camera->target[i]=ref[i]+p->offset_now[i];
    }
    int horizontal=sine12((unsigned)p->pitch_now+1024u);
    int x=scale12(sine12((unsigned)p->yaw_now),horizontal);
    int z=scale12(sine12((unsigned)p->yaw_now+1024u),horizontal);
    camera->eye[0]=camera->target[0]+scale12(x,camera->radius);
    camera->eye[1]=camera->target[1]+scale12(-sine12((unsigned)p->pitch_now),camera->radius);
    camera->eye[2]=camera->target[2]+scale12(z,camera->radius);
    camera->yaw=bfm_waypoint_heading(camera->eye[0]-camera->target[0],
                                     camera->eye[2]-camera->target[2]);
}
