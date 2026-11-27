/* SPDX-License-Identifier: MIT */
#include "woods_lily.h"
#include "field_waypoint.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { PADS = 3, PHASES = 32, Q12 = 4096 };
static const uint32_t origin = UINT32_C(0x80128158), room_root = UINT32_C(0x8018ea38);

typedef struct {
    int16_t home[3], position[3];
    unsigned phase, amplitude, heading, model_yaw;
} Lily;

struct BfmWoodsLilies {
    Lily pads[PADS];
    int16_t low[3], high[3];
    unsigned previous_rider;
    BfmWoodsLilyArt *art;
    BfmWoodsJumpPose jump;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes,
                              uint32_t address, size_t length)
{
    if (!scene || address < origin) return NULL;
    size_t offset = address - origin;
    return offset <= bytes && length <= bytes - offset ? scene + offset : NULL;
}

static int read_pads(BfmWoodsLilies *lilies, const uint8_t *scene, size_t bytes)
{
    const uint8_t *room = mounted(scene,bytes,room_root,0xbc);
    const uint8_t *profile = mounted(scene,bytes,UINT32_C(0x801c4200),12);
    if (!room || !profile) return 0;
    const uint8_t *shape = mounted(scene,bytes,u32le(profile+8),16);
    if (!shape || u32le(shape)) return 0;
    for (unsigned k = 0; k < 3; ++k) {
        lilies->low[k] = s16le(shape+4+4*k);
        lilies->high[k] = s16le(shape+6+4*k);
        if (lilies->low[k] > lilies->high[k]) return 0;
    }
    uint32_t start = u32le(room+0xb4), end = u32le(room+0xb8);
    if (end <= start || (end-start)%20) return 0;
    const uint8_t *row = mounted(scene,bytes,start,20);
    if (!row || !mounted(scene,bytes,start,end-start)) return 0;
    unsigned count = 0;
    for (size_t left = end-start; left >= 20; row += 20, left -= 20) {
        if (!u16le(row+6)) return count == PADS;
        if (u16le(row+6) != 148) continue;
        if (count == PADS) return 0;
        Lily *pad = &lilies->pads[count++];
        for (unsigned k = 0; k < 3; ++k) {
            pad->position[k] = pad->home[k] = s16le(row+2*k);
            if (pad->home[k] < INT16_MIN+16 || pad->home[k] > INT16_MAX-16) return 0;
        }
        pad->amplitude = 1;
        pad->model_yaw = u16le(row+12) & 4095u;
        if (pad->model_yaw) return 0;
        for (unsigned k = 0; k < 3; ++k)
            if (pad->home[k]+lilies->low[k] < INT16_MIN+18 ||
                pad->home[k]+lilies->high[k] > INT16_MAX-18) return 0;
    }
    return 0;
}

BfmWoodsLilies *bfm_woods_lilies_open(const MusashiDiscMedia *disc, const uint8_t *scene,
                                      size_t bytes)
{
    BfmWoodsLilies *lilies = calloc(1,sizeof *lilies);
    if (!lilies) return NULL;
    const uint8_t *room = mounted(scene,bytes,room_root,0xbc);
    if (read_pads(lilies,scene,bytes) &&
        bfm_woods_jump_pose_load(&lilies->jump,disc,u32le(room+0x4c))) {
        lilies->art = bfm_woods_lily_art_load(scene,bytes);
        if (lilies->art) return lilies;
    }
    bfm_woods_lilies_free(lilies);
    return NULL;
}

void bfm_woods_lilies_free(BfmWoodsLilies *lilies)
{
    if (lilies) bfm_woods_lily_art_free(lilies->art);
    free(lilies);
}

const BfmWoodsJumpPose *bfm_woods_lilies_jump_poses(const BfmWoodsLilies *lilies)
{
    return lilies ? &lilies->jump : NULL;
}

static int scaled(int64_t value, int divisor)
{
    return (int)(value / divisor - (value % divisor < 0));
}

static int sine(unsigned angle)
{
    return (int)lround(sin((angle & 4095u) * (6.28318530717958647693 / Q12)) * Q12);
}

void bfm_woods_lilies_tick(BfmWoodsLilies *lilies, unsigned rider, const int16_t feet[3],
                          int16_t carry[3])
{
    memset(carry,0,3*sizeof *carry);
    for (unsigned i = 0; i < PADS; ++i) {
        Lily *pad = &lilies->pads[i];
        if (rider == i+1 && rider != lilies->previous_rider) {
            /* The first contact rocks the pad toward the incoming player.
             * Its existing wave angle survives the impact. */
            pad->heading = bfm_waypoint_heading(pad->position[2]-feet[2], feet[0]-pad->position[0]);
            pad->amplitude = 16;
        }
        unsigned angle = pad->phase * (Q12 / PHASES);
        int lateral = (int)pad->amplitude * sine(angle+1024);
        int16_t position[3] = {
            (int16_t)(pad->home[0] + scaled((int64_t)lateral*sine(pad->heading+1024),Q12*Q12)),
            (int16_t)(pad->home[1] + scaled((int)pad->amplitude*sine(angle),Q12)),
            (int16_t)(pad->home[2] + scaled(-(int64_t)lateral*sine(pad->heading),Q12*Q12))
        };
        if (rider == i+1)
            for (unsigned k = 0; k < 3; ++k) carry[k] = position[k]-pad->position[k];
        memcpy(pad->position,position,sizeof position);
        pad->phase = (pad->phase + 1) % PHASES;
        if (!pad->phase && pad->amplitude > 1) --pad->amplitude;
    }
    lilies->previous_rider = rider;
}

static int within(const BfmWoodsLilies *lilies, const Lily *pad, int x, int z)
{
    x -= pad->position[0];
    z -= pad->position[2];
    return x >= lilies->low[0] && x <= lilies->high[0] &&
           z >= lilies->low[2] && z <= lilies->high[2];
}

unsigned bfm_woods_lilies_support(const BfmWoodsLilies *lilies, unsigned rider,
                                 const int16_t from[3], int16_t to[3])
{
    for (unsigned i = 0; i < PADS; ++i) {
        const Lily *pad = &lilies->pads[i];
        int top = pad->position[1] + lilies->low[1];
        if (rider == i+1) {
            if (!within(lilies,pad,to[0],to[2])) continue;
        } else {
            int dy = to[1]-from[1];
            /* Sweep to the mounted box top. The standing clearance applies
             * only after the descending segment actually reaches the face. */
            if (dy <= 0 || from[1] >= top || to[1] < top) continue;
            int x = from[0] + (int)((int64_t)(to[0]-from[0])*(top-from[1])/dy);
            int z = from[2] + (int)((int64_t)(to[2]-from[2])*(top-from[1])/dy);
            if (!within(lilies,pad,x,z)) continue;
        }
        to[1] = (int16_t)(top - 2);
        return i+1;
    }
    return 0;
}

void bfm_woods_lilies_shadow(const BfmWoodsLilies *lilies, BfmBedroomRoot *r)
{
    for (unsigned i = 0; i < PADS; ++i) {
        const Lily *pad = &lilies->pads[i];
        int surface = pad->position[1] + lilies->low[1];
        if (r->xyz[1] > surface || !within(lilies,pad,r->xyz[0],r->xyz[2]) ||
            (r->shadow_visible && r->shadow_xyz[1] <= surface)) continue;
        memcpy(r->shadow_xyz,r->xyz,sizeof r->shadow_xyz);
        r->shadow_xyz[1] = (int16_t)surface;
        r->shadow_normal[0] = r->shadow_normal[2] = 0;
        r->shadow_normal[1] = -4096;
        r->shadow_visible = 1;
    }
}

int bfm_woods_lilies_add(const BfmWoodsLilies *lilies, BfmBedroomFigure *figure,
                        const BfmBedroomView *view)
{
    if (!lilies) return 0;
    for (unsigned i = 0; i < PADS; ++i) {
        BfmForestTransform root;
        bfm_forest_actor_root(lilies->pads[i].model_yaw,lilies->pads[i].position,&root);
        if (!bfm_woods_lily_art_append(lilies->art,figure,view,&root)) return 0;
    }
    return 1;
}
