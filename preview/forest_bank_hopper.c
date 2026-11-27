/* SPDX-License-Identifier: MIT */
#include "forest_bank_hopper.h"
#include "field_waypoint.h"
#include "forest_guard_contact.h"
#include "village_ground.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { PARTS = 12, PLACES = 4 };
static const uint32_t origin = UINT32_C(0x80128158), room = UINT32_C(0x801910e4);
typedef enum { OUTSIDE, WAIT_CLEAR, APPEAR, REST, WAKE, SPIN } Phase;
typedef struct {
    int16_t home[3], xyz[3];
    unsigned home_yaw, yaw, travel_yaw, timer, age;
    int frac[2], fixed, bound;
    unsigned width, release_ticks, generation;
    Phase phase;
} Hopper;

struct BfmBankHoppers {
    const uint8_t *scene, *shadow;
    size_t bytes;
    MusashiForestPlayerMesh *mesh;
    BfmVillageGround *ground;
    BfmGameplayRun solo;
    BfmForestGuardBox box;
    struct { const uint8_t *rows; unsigned count; } clips[3];
    Hopper places[PLACES];
};

static const uint8_t *mounted(const BfmBankHoppers *h, uint32_t addr, size_t length)
{
    if (addr < origin || addr-origin > h->bytes || length > h->bytes-(addr-origin)) return NULL;
    return h->scene+(addr-origin);
}

static int placements(BfmBankHoppers *h, uint32_t list, unsigned *count, int fixed)
{
    for (;;) {
        const uint8_t *entry = mounted(h,list,20);
        if (!entry) return 0;
        unsigned type = u16le(entry+6);
        if (!type) return 1;
        if (type == 110) {
            if (*count >= PLACES || u16le(entry+8) != (fixed ? 0x801u : 1u)) return 0;
            Hopper *p = &h->places[(*count)++];
            p->fixed = fixed;
            for (unsigned j=0; j<3; ++j) p->home[j] = s16le(entry+2*j);
            p->home_yaw = u16le(entry+12)&4095;
        }
        list += 20;
    }
}

BfmBankHoppers *bfm_bank_hoppers_open(const uint8_t *scene, size_t bytes)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmBankHoppers *h = calloc(1,sizeof *h);
    if (!h) return NULL;
    h->scene = scene;
    h->bytes = bytes;
    const uint8_t *profile = mounted(h,UINT32_C(0x801bfe40),50);
    const uint8_t *field = mounted(h,room,0xbc);
    if (!profile || u16le(profile+44) != 12 || !field) goto fail;
    h->shadow = mounted(h,UINT32_C(0x8018def8),12);
    const uint8_t *shape = mounted(h,u32le(profile+8),16);
    if (!h->shadow || !shape || u32le(shape)) goto fail;
    for (unsigned i=0; i<3; ++i) {
        h->box.low[i] = s16le(shape+4+4*i);
        h->box.high[i] = s16le(shape+6+4*i);
        if (h->box.low[i] > h->box.high[i]) goto fail;
    }
    unsigned count = 0, parts = 0;
    if (!placements(h,u32le(field+0xb4),&count,1) ||
        !placements(h,u32le(field+0xb8),&count,0) || count != PLACES) goto fail;
    h->mesh = musashi_forest_player_mesh_from_table(scene,bytes,origin,u32le(profile+4),&parts);
    h->ground = bfm_village_ground_open(scene,bytes,room);
    if (!h->mesh || parts != PARTS || !h->ground) goto fail;
    static const uint32_t motion[] = {0x801c553cu,0x801c554cu,0x801c5614u};
    for (unsigned i=0; i<3; ++i) {
        h->clips[i].rows = mounted(h,motion[i],8);
        for (unsigned j=0;; ++j) {
            const uint8_t *row = mounted(h,motion[i]+8*j,8);
            if (!row || j > 255) goto fail;
            uint32_t pose = u32le(row);
            if (!pose) {
                if (!j || u32le(row+4) != 0xffff) goto fail;
                h->clips[i].count = j;
                break;
            }
            unsigned tag = pose >> 24;
            unsigned stride = tag == 0x81 ? 8 : 12;
            if ((tag != 0x80 && tag != 0x81) || u32le(row+4) != 1 ||
                !mounted(h,pose & ~UINT32_C(0x01000000),stride*PARTS)) goto fail;
        }
    }
    return h;
fail:
    bfm_bank_hoppers_free(h);
    return NULL;
}

void bfm_bank_hoppers_free(BfmBankHoppers *h)
{
    if (!h) return;
    musashi_forest_player_mesh_free(h->mesh);
    bfm_village_ground_free(h->ground);
    free(h);
}

static int64_t planar(const int16_t a[3], const int16_t b[3])
{
    int64_t x = a[0]-b[0], z = a[2]-b[2];
    return x*x+z*z;
}

static void aim(Hopper *p, const BfmBedroomRoot *player, BfmGameplayRun *run)
{
    if (planar(p->xyz,player->xyz) <= 384*384 && planar(p->home,p->xyz) < 1280*1280) {
        p->travel_yaw = bfm_waypoint_heading(p->xyz[0]-player->xyz[0],p->xyz[2]-player->xyz[2]);
    } else {
        p->travel_yaw = bfm_gameplay_random(run)&4095;
    }
}

static int move(BfmBankHoppers *h, Hopper *p)
{
    BfmForestTransform heading;
    bfm_forest_actor_root(p->travel_yaw,p->xyz,&heading);
    int16_t dst[3];
    memcpy(dst,p->xyz,sizeof dst);
    int remainder[2];
    for (unsigned i=0; i<2; ++i) {
        unsigned axis = 2*i;
        int q = p->frac[i]-8*heading.rotation[axis][2];
        int whole = q/4096-(q%4096 < 0);
        remainder[i] = q-4096*whole;
        int next = p->xyz[axis]+whole;
        if (next < INT16_MIN || next > INT16_MAX) return 0;
        dst[axis] = (int16_t)next;
    }
    int x=dst[0], z=dst[2];
    BfmForestContact floor;
    if (bfm_village_ground_step(h->ground,p->xyz,dst,&floor) != BFM_VILLAGE_GROUND_FOOTING ||
        dst[0] != x || dst[2] != z) return 0;
    memcpy(p->xyz,dst,sizeof dst);
    memcpy(p->frac,remainder,sizeof remainder);
    return 1;
}

static int overlaps(const BfmBankHoppers *h, const Hopper *p, const BfmBedroomRoot *player)
{
    for (unsigned i=0; i<3; ++i) {
        int delta = player->xyz[i]-p->xyz[i]-(i==1 ? 42 : 0);
        if (delta < h->box.low[i] || delta > h->box.high[i]) return 0;
    }
    return 1;
}

unsigned bfm_bank_hoppers_step_target(BfmBankHoppers *h, const BfmBedroomRoot *player,
                                      const int16_t aim_point[3], BfmGameplayRun *run,
                                      unsigned *attack_yaw)
{
    if (!h || !player || !attack_yaw) return 0;
    unsigned power = 0;
    if (!run) run = &h->solo;
    for (unsigned i=0; i<PLACES; ++i) {
        Hopper *p = &h->places[i];
        if (p->phase == OUTSIDE) {
            int64_t d = planar(p->home,player->xyz);
            if (!p->fixed && (d <= 1536*1536 || d >= 1600*1600)) continue;
            memcpy(p->xyz,p->home,sizeof p->xyz);
            p->yaw = p->home_yaw;
            p->age = 0;
            p->frac[0] = p->frac[1] = 0;
            p->bound = 0;
            p->width = 4096;
            p->release_ticks = p->timer = 0;
            p->phase = WAIT_CLEAR;
            continue;
        }
        int64_t distance = planar(p->xyz,player->xyz);
        if (!p->fixed && distance >= 2048*2048) { p->phase = OUTSIDE; continue; }
        if (p->bound) continue;
        if (p->release_ticks) {
            p->width += (4096-p->width)/4;
            if (!--p->release_ticks) {
                p->width = 4096;
                p->age = 0;
                p->phase = REST;
            }
            continue;
        }
        switch (p->phase) {
        case WAIT_CLEAR:
            if (p->timer) { --p->timer; break; }
            if (distance >= 320*320) {
                if (!++p->generation) ++p->generation;
                p->phase = APPEAR;
                p->age = 0;
            }
            break;
        case APPEAR:
            if (++p->age == 16) { p->phase = REST; p->age = 0; }
            break;
        case REST:
            if (aim_point) {
                int64_t height = p->xyz[1]-aim_point[1];
                if (planar(p->xyz,aim_point)+height*height <= 0x18000) {
                    p->phase = WAKE;
                    p->age = 0;
                }
            }
            break;
        case WAKE:
            if (++p->age == h->clips[1].count) {
                p->phase = SPIN;
                p->age = 0;
                p->timer = 45;
                aim(p,player,run);
            }
            break;
        case SPIN:
            p->age = (p->age+1)%h->clips[2].count;
            p->yaw = (p->yaw-113u)&4095;
            if (overlaps(h,p,player) && !power) { *attack_yaw=p->travel_yaw; power=13; }
            if (!move(h,p)) {
                unsigned jitter = bfm_gameplay_random(run)&511;
                p->travel_yaw = (p->travel_yaw+2048u+
                                    ((bfm_gameplay_random(run)&1) ? jitter : 0u-jitter))&4095;
            } else if (--p->timer == 0) {
                aim(p,player,run);
                p->timer = 35;
            }
            break;
        default: break;
        }
    }
    return power;
}

unsigned bfm_bank_hoppers_step(BfmBankHoppers *h, const BfmBedroomRoot *player,
                               BfmGameplayRun *run, unsigned *attack_yaw)
{
    return bfm_bank_hoppers_step_target(h,player,player ? player->xyz : NULL,run,attack_yaw);
}

int bfm_bank_hoppers_draw(const BfmBankHoppers *h, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    for (unsigned i=0; i<PLACES; ++i) {
        const Hopper *p = &h->places[i];
        if (p->phase < APPEAR) continue;
        unsigned clip = p->phase == SPIN ? 2 : p->phase == WAKE ? 1 : 0;
        const uint8_t *row = h->clips[clip].rows+8*(p->age % h->clips[clip].count);
        uint32_t key = u32le(row);
        BfmForestPose pose;
        if (key & UINT32_C(0x01000000)) {
            if (!bfm_forest_actor_pose_decode(mounted(h,key & ~UINT32_C(0x01000000),8*PARTS),
                                              8*PARTS,PARTS,&pose)) return 0;
        } else if (!bfm_forest_actor_pose_decode_full(mounted(h,key,12*PARTS),
                                                      12*PARTS,PARTS,&pose)) return 0;
        BfmForestTransform root;
        bfm_forest_actor_root(p->yaw,p->xyz,&root);
        if (p->phase == APPEAR) {
            int scale = (int)lround(sin(p->age*(1.5707963267948966/16))*4096);
            for (unsigned r=0; r<3; ++r)
                for (unsigned c=0; c<3; ++c) root.rotation[r][c] = root.rotation[r][c]*scale/4096;
        }
        if (p->bound || p->release_ticks)
            for (unsigned r=0; r<3; ++r) {
                root.rotation[r][0] = root.rotation[r][0]*(int)p->width/4096;
                root.rotation[r][2] = root.rotation[r][2]*(int)p->width/4096;
            }
        BfmBedroomRoot ground = {0};
        memcpy(ground.xyz,p->xyz,sizeof ground.xyz);
        if (!bfm_bedroom_travel_air_shadow(h->scene,h->bytes,room,&ground)) return 0;
        BfmForestActor actor = {.mesh=h->mesh,.pose=&pose,.root=&root,.visible_parts=(1u<<PARTS)-1};
        if (!bfm_bedroom_figure_add_actor(figure,view,&actor,&ground,h->shadow,12288)) return 0;
    }
    return 1;
}

static const Hopper *target(const BfmBankHoppers *h, int id)
{
    if (!h || id < 1 || id > PLACES) return NULL;
    const Hopper *p = &h->places[id-1];
    return p->phase >= REST && !p->release_ticks ? p : NULL;
}

unsigned bfm_bank_hoppers_generation(const BfmBankHoppers *h, int id)
{
    const Hopper *p = target(h,id);
    return p ? p->generation : 0;
}

int bfm_bank_hoppers_target(const BfmBankHoppers *h, int id, int16_t center[3])
{
    const Hopper *p = target(h,id);
    if (!p || !center) return 0;
    BfmForestTransform tf;
    bfm_forest_actor_root(p->yaw,p->xyz,&tf);
    return bfm_forest_guard_contact(&h->box,&tf,p->xyz,p->xyz,center);
}

int bfm_bank_hoppers_root(const BfmBankHoppers *h, int id, int16_t xyz[3])
{
    const Hopper *p = target(h,id);
    if (!p || !xyz) return 0;
    memcpy(xyz,p->xyz,sizeof p->xyz);
    return 1;
}

int bfm_bank_hoppers_contact(const BfmBankHoppers *h, const int16_t from[3],
                             const int16_t to[3], int16_t hit[3])
{
    if (!h || !from || !to || !hit) return 0;
    int chosen = 0;
    int64_t nearest = INT64_MAX;
    for (int id=1; id<=PLACES; ++id) {
        const Hopper *p = target(h,id);
        if (!p || p->bound) continue;
        BfmForestTransform tf;
        bfm_forest_actor_root(p->yaw,p->xyz,&tf);
        int16_t center[3], impact[3];
        if (!bfm_forest_guard_contact_point(&h->box,&tf,from,to,center,impact,NULL)) continue;
        int64_t dy = from[1]-impact[1];
        int64_t range = planar(from,impact)+dy*dy;
        if (range < nearest) {
            memcpy(hit,impact,sizeof impact);
            nearest=range;
            chosen=id;
        }
    }
    return chosen;
}

int bfm_bank_hoppers_latch(BfmBankHoppers *h, int id)
{
    const Hopper *p = target(h,id);
    if (!p || p->bound) return 0;
    h->places[id-1].bound = 1;
    return 1;
}

void bfm_bank_hoppers_meter(BfmBankHoppers *h, int id, unsigned meter)
{
    const Hopper *p = target(h,id);
    if (!p || !p->bound) return;
    /* The held creature narrows horizontally while retaining its pose and height. */
    int scale = meter >= 128 ? 0 : (int)lround(cos(meter*(1.5707963267948966/128))*4096);
    h->places[id-1].width = scale > 0 ? (unsigned)scale : 1;
}

void bfm_bank_hoppers_release(BfmBankHoppers *h, int id)
{
    const Hopper *p = target(h,id);
    if (!p || !p->bound) return;
    h->places[id-1].bound = 0;
    h->places[id-1].release_ticks = 6;
}

int bfm_bank_hoppers_absorb(BfmBankHoppers *h, int id)
{
    const Hopper *active = target(h,id);
    if (!active || !active->bound) return 0;
    Hopper *p = &h->places[id-1];
    p->bound = 0;
    p->phase = WAIT_CLEAR;
    p->timer = 60;
    p->width = 4096;
    p->yaw = p->home_yaw;
    p->frac[0] = p->frac[1] = 0;
    memcpy(p->xyz,p->home,sizeof p->xyz);
    return 1;
}
