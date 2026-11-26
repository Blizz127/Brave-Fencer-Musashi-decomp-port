/* SPDX-License-Identifier: MIT */
#include "village_impact.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>

enum { PUFFS = 48, STARS = 6, LIFE = 32 };
typedef struct {
    int64_t start[3]; /* Q16 world units */
    int32_t velocity[3];
    int scale;
} Particle;

typedef struct {
    int left, top, width, height;
    unsigned u, v;
    BfmForestRasterMaterial material;
} Sprite;

struct BfmVillageImpact {
    Particle particles[BFM_VILLAGE_IMPACT_CARDS];
    Sprite dust, star;
    int center[3], radial_y, radial_z, shake[LIFE];
    unsigned age;
    int started;
};

static int32_t signed_fixed(uint32_t word)
{
    return (int32_t)((int64_t)(word ^ UINT32_C(0x80000000)) - INT64_C(0x80000000));
}

static int down(int64_t value, int divisor)
{
    return (int)(value / divisor - (value % divisor < 0));
}

static int sprite(Sprite *out, const uint8_t *card, unsigned tx, unsigned ty,
                  unsigned palette_y, int translucent)
{
    tx += u16le(card + 4);
    ty += u16le(card + 6);
    int width = card[2], height = card[3];
    unsigned u = (tx % 64) * 4, v = ty % 256;
    if (!width || !height || tx >= 1024 || ty >= 512 ||
        u + width > 256 || v + height > 256) return 0;
    *out = (Sprite){
        .width = width, .height = height, .left = s16le(card+8), .top = s16le(card+10),
        .u = u, .v = v, .material = {.page = (uint16_t)(tx/64 + (ty/256)*16 + 32),
        .clut = (uint16_t)(palette_y*64 + 352/16), .count = 4, .textured = 1,
        .translucent = translucent}
    };
    return 1;
}

BfmVillageImpact *bfm_village_impact_open(const uint8_t *scene, size_t scene_bytes,
                                         const uint8_t *main, size_t main_bytes)
{
    const uint32_t base = UINT32_C(0x80128158);
    const size_t profile = UINT32_C(0x80189804) - base;
    const size_t radial = UINT32_C(0x80189828) - base;
    const size_t smoke = UINT32_C(0x800d387c) - UINT32_C(0x800cedf8);
    if (!scene || scene_bytes < radial + 6 || !main || main_bytes < smoke + 12)
        return NULL;
    BfmVillageImpact *p = calloc(1, sizeof *p);
    if (!p) return NULL;
    if (!sprite(&p->dust, main+smoke, 624, 288, 412, 1) ||
        !sprite(&p->star, scene+(UINT32_C(0x801893dc)-base), 576, 464, 340, 0)) goto fail;
    for (unsigned k = 0; k < 3; ++k)
        p->center[k] = s16le(scene+(UINT32_C(0x801897fc)-base)+2*k);
    p->radial_y = s16le(scene+radial+2);
    p->radial_z = s16le(scene+radial+4);
    for (unsigned i = 0; i < STARS; ++i) {
        const uint8_t *record = scene+(UINT32_C(0x80189334)-base)+28*i;
        Particle *star = &p->particles[PUFFS+i];
        for (unsigned k = 0; k < 3; ++k) {
            star->start[k] = signed_fixed(u32le(record+4*k));
            star->velocity[k] = signed_fixed(u32le(record+12+4*k));
        }
        star->scale = s16le(record+24);
        if (star->scale <= 0) goto fail;
    }
    unsigned age = 0;
    for (unsigned i = 0; i <= LIFE/2; ++i) {
        const uint8_t *entry = scene+profile+2*i;
        if (!entry[1]) {
            if (age == LIFE && !entry[0]) return p;
            break;
        }
        unsigned duration = 1u + entry[1];
        if (duration > LIFE-age) break;
        int y = (int)(entry[0] ^ 128u) - 128;
        while (duration--) p->shake[age++] = y;
    }
fail:
    free(p);
    return NULL;
}

void bfm_village_impact_free(BfmVillageImpact *p)
{
    free(p);
}

static void scatter(BfmVillageImpact *p, BfmGameplayRun *run)
{
    const int16_t zero[3] = {0};
    for (unsigned bank = 0; bank < 6; ++bank) {
        int radius = 64 + (bfm_gameplay_random(run) & 63);
        int center[3] = {p->center[0], p->center[1], p->center[2]};
        center[1] -= (int)(bank < 3 ? bank : 5-bank) * 96;
        if (bank >= 3) center[2] -= 96;
        for (unsigned i = 0; i < 8; ++i) {
            unsigned pitch = 3840 + 8 * (bfm_gameplay_random(run) & 127u);
            unsigned yaw = (bank < 2 ? 3408u : 3840u) +
                           ((bfm_gameplay_random(run) & 0x7f00u) >> 5);
            BfmForestTransform axes;
            bfm_forest_actor_pitch_yaw(pitch, yaw, zero, &axes);
            Particle *dust = &p->particles[bank*8+i];
            for (unsigned k = 0; k < 3; ++k) {
                int d = down(radius*axes.rotation[k][0] +
                             p->radial_y*axes.rotation[k][1] + p->radial_z*axes.rotation[k][2],4096);
                dust->start[k] = (int64_t)(center[k]+d)*65536;
                dust->velocity[k] = d*4096;
            }
            dust->scale = 7*4096;
        }
    }
}

int bfm_village_impact_step(BfmVillageImpact *p, int hit, BfmGameplayRun *run)
{
    if (!p) return 0;
    if (p->started) {
        if (p->age <= LIFE) ++p->age;
        return 0;
    }
    if (!hit || !run) return 0;
    scatter(p, run);
    p->started = 1;
    return 1;
}

int bfm_village_impact_shake_y(const BfmVillageImpact *p)
{
    return p && p->age && p->age <= LIFE ? p->shake[p->age-1] : 0;
}

static int project_card(const Particle *part, const Sprite *card, unsigned slot, unsigned age,
                        const BfmBedroomView *view, const BfmForestTransform *spin,
                        BfmForestDustCard *face)
{
    int16_t xyz[3];
    for (unsigned k = 0; k < 3; ++k) {
        int coordinate = down(part->start[k] + (int64_t)age*part->velocity[k],65536);
        if (coordinate < INT16_MIN || coordinate > INT16_MAX) return 0;
        xyz[k] = (int16_t)coordinate;
    }
    int xy[3];
    if (!bfm_bedroom_project(view, xyz, xy) || xy[2] >= 16380) return 0;
    int reciprocal = (int)(((int64_t)view->focal*65536 + xy[2]/2)/xy[2]);
    if (reciprocal > 131071) reciprocal = 131071;
    /* Size and local origin are truncated to whole world units before the
     * perspective division, including the half-size stars' odd widths. */
    int x = down((int64_t)reciprocal*down(card->left*part->scale,4096),65536);
    int y = down((int64_t)reciprocal*down(card->top*part->scale,4096),65536);
    int w = down((int64_t)reciprocal*down(card->width*part->scale,4096),65536);
    int h = down((int64_t)reciprocal*down(card->height*part->scale,4096),65536);
    face->bucket = (unsigned)xy[2]/4 + 1;
    face->slot = slot;
    face->material = card->material;
    unsigned shade = slot < PUFFS ? 144-2*age : 128;
    for (unsigned j = 0; j < 4; ++j) {
        int right = j & 1, bottom = j >> 1;
        int dx = x+right*w, dy = y+bottom*h;
        if (slot >= PUFFS) {
            int cosine = spin->rotation[0][0], sine = spin->rotation[0][2];
            int rx = down((int64_t)cosine*dx - (int64_t)sine*dy,4096);
            dy = down((int64_t)sine*dx + (int64_t)cosine*dy,4096);
            dx = rx;
        }
        face->vertices[j] = (BfmForestRasterVertex){.x = xy[0]+dx, .y = xy[1]+dy,
            .u = (uint8_t)(card->u+right*(card->width-1)),
            .v = (uint8_t)(card->v+bottom*(card->height-1)), .rgb = shade*0x010101u};
    }
    return 1;
}

unsigned bfm_village_impact_project(const BfmVillageImpact *p, const BfmBedroomView *view,
                                    BfmForestDustCard out[BFM_VILLAGE_IMPACT_CARDS])
{
    if (!p || !p->started || p->age > LIFE || !view || !out ||
        view->focal <= 0 || view->focal > INT16_MAX) return 0;
    const int16_t zero[3] = {0};
    BfmForestTransform spin;
    bfm_forest_actor_root(341*p->age,zero,&spin);
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_VILLAGE_IMPACT_CARDS; ++i) {
        const Sprite *card = i < PUFFS ? &p->dust : &p->star;
        count += project_card(&p->particles[i],card,i,p->age,view,&spin,&out[count]);
    }
    return count;
}
