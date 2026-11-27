/* SPDX-License-Identifier: MIT */
#include "woods_caster_ring.h"
#include "forest_guard_contact.h"
#include "../pc_port/forest_player_bytes.h"

#include <math.h>
#include <stdlib.h>

/* Two casters can each have three pairs in flight before the next cast. */
enum { RINGS = 12, RIM = 12, STAGES = 3, UNIT = 4096 };

typedef struct {
    unsigned used, fresh, yaw, stage, scale;
    int remaining;
    int64_t position[3];
    int velocity[3];
} Ring;

struct BfmWoodsCasterRings {
    Ring pool[RINGS];
    const uint8_t *hands, *styles, *chords;
    BfmForestGuardBox player;
    BfmWoodsCasterRingHit hit;
};

static int floor_div(int64_t n, int divisor)
{
    return (int)(n / divisor) - (n % divisor < 0);
}

BfmWoodsCasterRings *bfm_woods_rings_load(const uint8_t *scene, size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158);
    const uint32_t body = UINT32_C(0x801891c0) - base;
    const uint32_t styles = UINT32_C(0x801c4770) - base;
    const uint32_t hands = UINT32_C(0x801c4830) - base;
    const uint32_t chords = UINT32_C(0x801c495c) - base;
    if (!scene || bytes < chords + 8*8 || u32le(scene+body)) return NULL;
    BfmWoodsCasterRings *rings = calloc(1,sizeof(*rings));
    if (!rings) return NULL;
    rings->styles = scene+styles;
    rings->hands = scene+hands;
    rings->chords = scene+chords;
    for (unsigned k = 0; k < 3; ++k) {
        rings->player.low[k] = s16le(scene+body+4+4*k);
        rings->player.high[k] = s16le(scene+body+6+4*k);
        if (rings->player.low[k] > rings->player.high[k]) {
            free(rings);
            return NULL;
        }
    }
    return rings;
}

void bfm_woods_rings_free(BfmWoodsCasterRings *rings)
{
    free(rings);
}

static void transform(const BfmForestTransform *root, int x, int y, int z, int16_t world[3])
{
    for (unsigned i = 0; i < 3; ++i) {
        const int16_t *r = root->rotation[i];
        int64_t dot = (int64_t)r[0]*x + (int64_t)r[1]*y + (int64_t)r[2]*z;
        world[i] = (int16_t)(root->translation[i] + floor_div(dot,UNIT));
    }
}

void bfm_woods_rings_emit(BfmWoodsCasterRings *rings, const int16_t feet[3],
                          unsigned yaw, unsigned stage, unsigned option)
{
    if (!rings || !feet || stage >= STAGES) return;
    BfmForestTransform caster;
    bfm_forest_actor_root(yaw,feet,&caster);
    for (unsigned hand = 0, slot = 0; hand < 2; ++hand, ++slot) {
        while (slot < RINGS && rings->pool[slot].used) ++slot;
        if (slot == RINGS) break;
        Ring *ring = &rings->pool[slot];
        *ring = (Ring){.used=1,.fresh=1,.yaw=yaw,.stage=stage,.scale=UNIT,.remaining=60};
        const uint8_t *joint = rings->hands + 8*hand;
        int16_t point[3];
        transform(&caster,s16le(joint),s16le(joint+2),s16le(joint+4),point);
        for (unsigned i = 0; i < 3; ++i) {
            ring->position[i] = (int64_t)point[i]*65536;
            ring->velocity[i] = caster.rotation[i][2] * (option ? -24 : -72);
        }
    }
}

static void ring_root(const Ring *ring, BfmForestTransform *out, int16_t point[3])
{
    for (unsigned i = 0; i < 3; ++i) point[i] = (int16_t)floor_div(ring->position[i],65536);
    bfm_forest_actor_root(ring->yaw,point,out);
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned axis = 0; axis < 2; ++axis)
            out->rotation[i][axis] = (int16_t)floor_div((int64_t)out->rotation[i][axis]*ring->scale,UNIT);
    }
}

static int touches(const BfmWoodsCasterRings *rings, const BfmForestTransform *root,
                   const BfmForestTransform *player)
{
    for (unsigned i = 0; i < 4; ++i) {
        const uint8_t *p = rings->chords + i*8;
        int16_t a[3], b[3], center[3];
        transform(root,s16le(p),s16le(p+2),s16le(p+4),a);
        p += 32;
        transform(root,s16le(p),s16le(p+2),s16le(p+4),b);
        if (bfm_forest_guard_contact(&rings->player,player,a,b,center)) return 1;
    }
    return 0;
}

static void hit_normal(const int16_t source[3], const int16_t feet[3], int16_t out[3])
{
    int delta[3];
    unsigned length2 = 0;
    for (unsigned i = 0; i < 3; ++i) {
        delta[i] = feet[i]-source[i];
        length2 += (unsigned)(delta[i]*delta[i]);
    }
    if (!length2) return;
    /* The field normalizer quantizes squared distance to a base-four mantissa. */
    int divisor = 8;
    while (length2 < 64) { length2 *= 4; divisor /= 2; }
    while (length2 > 255) { length2 >>= 2; divisor *= 2; }
    int gain = (int)sqrt((double)(1u << 30)/length2);
    for (unsigned i = 0; i < 3; ++i) out[i] = (int16_t)floor_div(delta[i]*gain,divisor);
}

void bfm_woods_rings_step(BfmWoodsCasterRings *rings, const int16_t feet[3],
                          int mini, unsigned option)
{
    if (!rings || !feet) return;
    rings->hit = (BfmWoodsCasterRingHit){0};
    BfmForestTransform player;
    bfm_forest_actor_root(0,feet,&player);
    for (unsigned i = 0; i < RINGS; ++i) {
        Ring *r = &rings->pool[i];
        if (!r->used) continue;
        if (r->fresh) {
            r->fresh = 0;
            continue;
        }
        BfmForestTransform root;
        int16_t center[3];
        ring_root(r,&root,center);
        if (!(mini && option) && touches(rings,&root,&player)) {
            rings->hit = (BfmWoodsCasterRingHit){
                .flags=mini ? 0x4001u : 0x5001u, .yaw=r->yaw,
                .power=mini ? 10u : 10u*(option^1u),
                .point={feet[0],(int16_t)(feet[1]-32),feet[2]}
            };
            hit_normal(center,feet,rings->hit.normal);
        }
        /* Contacts precede expiration and movement; they do not consume the ring. */
        if (r->remaining-- < 0) {
            r->used = 0;
            continue;
        }
        r->scale += 256;
        for (unsigned axis = 0; axis < 3; ++axis) r->position[axis] += r->velocity[axis];
    }
}

const BfmWoodsCasterRingHit *bfm_woods_rings_hit(const BfmWoodsCasterRings *rings)
{
    return rings && rings->hit.flags ? &rings->hit : NULL;
}

static int add_ring(const BfmWoodsCasterRings *rings, const Ring *ring,
                    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    BfmForestTransform root;
    int16_t point[3];
    int center[3];
    ring_root(ring,&root,point);
    if (!bfm_bedroom_project(view,point,center)) return 1;
    const uint8_t *style = rings->styles + 64*ring->stage;
    BfmForestRasterVertex outer[RIM], inner[RIM];
    for (unsigned k = 0; k < RIM; ++k) {
        const uint8_t *vertex = style + 16 + 4*k;
        int projected[3];
        transform(&root,s16le(vertex),s16le(vertex+2),0,point);
        bfm_bedroom_project(view,point,projected);
        outer[k] = (BfmForestRasterVertex){.x=projected[0],.y=projected[1],.rgb=u32le(style+4)&0xffffff};
        inner[k] = (BfmForestRasterVertex){.x=center[0]+floor_div(3*(projected[0]-center[0]),4),
            .y=center[1]+floor_div(3*(projected[1]-center[1]),4),.rgb=u32le(style)&0xffffff};
    }
    BfmForestDustCard cards[RIM];
    for (unsigned k = 0; k < RIM; ++k) {
        unsigned next = (k+1)%RIM;
        cards[k] = (BfmForestDustCard){.vertices={outer[k],outer[next],inner[k],inner[next]},
            .bucket=(unsigned)center[2]/4+1, .material={.count=4,.translucent=1}};
    }
    return bfm_bedroom_figure_add_fusion(figure,view,NULL,cards,RIM);
}

int bfm_woods_rings_add(const BfmWoodsCasterRings *rings, BfmBedroomFigure *figure,
                        const BfmBedroomView *view)
{
    if (!rings || !figure || !view) return 0;
    for (unsigned i = 0; i < RINGS; ++i)
        if (rings->pool[i].used && !add_ring(rings,&rings->pool[i],figure,view)) return 0;
    return 1;
}

int bfm_woods_ring_mini_active(const BfmForestStatus *status)
{
    return status && (status->record[0x23] & 0x80) != 0;
}

void bfm_woods_ring_mini_apply(BfmForestStatus *status)
{
    if (!status) return;
    status->record[0x20] = status->record[0x21] = status->record[0x22] = 0;
    status->record[0x23] = 0x80;
}

void bfm_woods_ring_mini_step(BfmForestStatus *status)
{
    if (!bfm_woods_ring_mini_active(status)) return;
    uint32_t timer = u32le(status->record+0x20) + 5461;
    if (((timer >> 16) & 0x7fff) >= 38) timer = 0;
    for (unsigned i = 0; i < 4; ++i) status->record[0x20+i] = (uint8_t)(timer >> (8*i));
}
