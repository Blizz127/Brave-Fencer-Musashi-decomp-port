/* SPDX-License-Identifier: MIT */
#include "woods_caster_burst.h"
#include "hero_smoke_material.h"
#include "village_ground.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { PUFFS_PER_DEATH = 10, PIECES_PER_DEATH = 3, ACTOR_SLOTS = 96 };
typedef struct {
    int16_t xyz[3];
    unsigned remaining;
} Puff;
typedef enum { EMPTY, WAIT, FALLING, LANDED, DISSOLVING } Phase;
typedef struct {
    int16_t xyz[3];
    int16_t sway[2];
    unsigned yaw, kind, timer, sway_phase;
    int pitch, y_fraction, gravity, vy;
    Phase phase;
} Piece;
struct BfmWoodsCasterBurst {
    BfmHeroSmokeMaterial smoke;
    BfmVillageGround *ground;
    MusashiForestPlayerMesh *body, *third_mesh;
    unsigned parts[PIECES_PER_DEATH], gravity[PIECES_PER_DEATH];
    BfmForestPose detach, identity;
    Puff puffs[ACTOR_SLOTS];
    Piece pieces[ACTOR_SLOTS];
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t size)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (address < origin || address-origin > bytes || size > bytes-(address-origin)) return NULL;
    return scene+(address-origin);
}

BfmWoodsCasterBurst *bfm_woods_caster_burst_open(const uint8_t *scene, size_t bytes,
                                                 const uint8_t *main, size_t main_bytes)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (!scene || !main || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    const uint8_t *profile = mounted(scene,bytes,UINT32_C(0x801c473c),8);
    const uint8_t *pieces = mounted(scene,bytes,UINT32_C(0x801c49c4),44);
    const uint8_t *clip = mounted(scene,bytes,UINT32_C(0x801c4840),16);
    if (!profile || !pieces || !clip || (u32le(clip)>>24) != 0x81 ||
        u32le(clip+4) != 1 || u32le(clip+8) || u32le(clip+12) != 0xffff) return NULL;
    const uint8_t *frame = mounted(scene,bytes,u32le(clip)&~UINT32_C(0x01000000),32);
    if (!frame) return NULL;
    BfmWoodsCasterBurst *burst = calloc(1,sizeof *burst);
    if (!burst) return NULL;
    const uint8_t *body = mounted(scene,bytes,u32le(profile+4),20);
    unsigned count;
    if (!body || !bfm_hero_run_smoke_load(&burst->smoke,main,main_bytes) ||
        !bfm_forest_actor_pose_decode(frame,32,4,&burst->detach)) goto fail;
    burst->body = musashi_forest_player_mesh_from_table(scene,bytes,origin,u32le(profile+4),&count);
    if (!burst->body || count != 4) goto fail;
    for (unsigned i = 0; i < PIECES_PER_DEATH; ++i) {
        const uint8_t *table = mounted(scene,bytes,u32le(pieces+4*i),8);
        unsigned part = u32le(pieces+12+4*i), gravity = u32le(pieces+24+4*i);
        if (!table || part >= count || gravity > 65535 || !u32le(table) || u32le(table+4)) goto fail;
        if (i < 2 && u32le(body+part*4) != u32le(table)) goto fail;
        burst->parts[i] = part;
        burst->gravity[i] = gravity;
    }
    /* The third piece is a separate mounted mesh. The first two select
     * individual parts of the same four-piece caster already parsed above. */
    burst->third_mesh = musashi_forest_player_mesh_from_table(scene,bytes,origin,u32le(pieces+8),&count);
    if (!burst->third_mesh || count != 1) goto fail;
    for (unsigned i = 0; i < 4; ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            burst->identity.local[i].rotation[axis][axis] = 4096;
    burst->ground = bfm_village_ground_open(scene,bytes,UINT32_C(0x8018ea38));
    if (burst->ground) return burst;
fail:
    bfm_woods_caster_burst_free(burst);
    return NULL;
}

void bfm_woods_caster_burst_free(BfmWoodsCasterBurst *burst)
{
    if (!burst) return;
    musashi_forest_player_mesh_free(burst->body);
    musashi_forest_player_mesh_free(burst->third_mesh);
    bfm_village_ground_free(burst->ground);
    free(burst);
}

static int down(int64_t value, int divisor)
{
    return (int)(value/divisor-(value%divisor < 0));
}

static int sine(unsigned phase)
{
    return (int)lround(4096.0*sin((phase & 4095u)*(6.28318530717958647693/4096.0)));
}

static int16_t signed_word(int value)
{
    unsigned low = (unsigned)value & 65535u;
    return (int16_t)(low <= INT16_MAX ? (int)low : (int)low-65536);
}

int bfm_woods_caster_burst_start(BfmWoodsCasterBurst *burst, const BfmBedroomRoot *caster,
                                 BfmGameplayRun *run)
{
    if (!burst || !caster || !run) return 0;
    unsigned slot = 0;
    for (unsigned i = 0; i < PUFFS_PER_DEATH; ++i) {
        unsigned heading = bfm_gameplay_random(run) & 4095u;
        int radius = bfm_gameplay_random(run)%48;
        int dy = bfm_gameplay_random(run)%24;
        if (!(bfm_gameplay_random(run)&1)) dy = -dy;
        while (slot < ACTOR_SLOTS && burst->puffs[slot].remaining) ++slot;
        if (slot == ACTOR_SLOTS) continue;
        Puff *p = &burst->puffs[slot++];
        p->xyz[0] = signed_word(caster->xyz[0]+down(radius*sine(heading),4096));
        p->xyz[1] = signed_word(caster->xyz[1]-52+dy);
        p->xyz[2] = signed_word(caster->xyz[2]+down(radius*sine(heading+1024),4096));
        p->remaining = BFM_HERO_SMOKE_CELLS;
    }
    BfmForestTransform root;
    bfm_forest_actor_root(caster->yaw,caster->xyz,&root);
    slot = 0;
    for (unsigned i = 0; i < PIECES_PER_DEATH; ++i) {
        while (slot < ACTOR_SLOTS && burst->pieces[slot].phase != EMPTY) ++slot;
        if (slot == ACTOR_SLOTS) break;
        Piece *piece = &burst->pieces[slot++];
        /* Parent-owned pieces occupy later slots and receive the first
         * of eight waiting updates on their birth field. */
        *piece = (Piece){.kind=i,.phase=WAIT,.timer=7,.yaw=caster->yaw};
        const int32_t *offset = burst->detach.local[burst->parts[i]].translation;
        for (unsigned axis = 0; axis < 3; ++axis) {
            int sum = 0;
            for (unsigned k = 0; k < 3; ++k) sum += root.rotation[axis][k]*offset[k];
            piece->xyz[axis] = signed_word(root.translation[axis]+down(sum,4096));
        }
    }
    return 1;
}

static int advance_piece(BfmWoodsCasterBurst *burst, Piece *p, BfmGameplayRun *run)
{
    if (p->phase == WAIT) {
        if (!--p->timer) {
            p->gravity = (int)burst->gravity[p->kind]+(bfm_gameplay_random(run)&1023);
            p->sway_phase = bfm_gameplay_random(run);
            p->phase = FALLING;
            p->timer = 256;
        }
        return 1;
    }
    if (p->phase == FALLING) {
        if (p->kind == 2) {
            if (p->pitch > -1024) p->pitch -= 16;
        } else {
            int wave = sine(1024+64*p->sway_phase++);
            BfmForestTransform rotation;
            bfm_forest_actor_root(p->yaw,p->xyz,&rotation);
            p->sway[0] = (int16_t)down((int64_t)rotation.rotation[0][0]*wave,4096*256);
            p->sway[1] = (int16_t)down((int64_t)rotation.rotation[2][0]*wave,4096*256);
        }
        p->vy += p->gravity;
        int fixed = p->y_fraction+p->vy;
        int step = down(fixed,65536);
        p->y_fraction = fixed-step*65536;
        int16_t next[3] = {p->xyz[0],signed_word((int)p->xyz[1]+step),p->xyz[2]};
        BfmForestContact floor;
        BfmVillageGroundResult contact = bfm_village_ground_air(burst->ground,p->xyz,next,&floor);
        if (contact == BFM_VILLAGE_GROUND_BLOCKED) return 0;
        memcpy(p->xyz,next,sizeof p->xyz);
        if (contact == BFM_VILLAGE_GROUND_FOOTING) {
            p->timer = 32;
            p->phase = LANDED;
        } else if (!--p->timer) {
            p->phase = EMPTY;
        }
        return 1;
    }
    if (p->phase == LANDED) {
        if (p->kind == 2 && p->pitch > -1024) {
            p->pitch -= 128;
            if (p->pitch < -1024) p->pitch = -1024;
        }
        if (!--p->timer) {
            p->phase = DISSOLVING;
            p->timer = 8;
        }
    } else if (p->phase == DISSOLVING && !--p->timer) {
        p->phase = EMPTY;
    }
    return 1;
}

int bfm_woods_caster_burst_tick(BfmWoodsCasterBurst *burst, BfmGameplayRun *run)
{
    if (!burst || !run) return 0;
    for (unsigned i = 0; i < ACTOR_SLOTS; ++i) {
        if (burst->puffs[i].remaining) --burst->puffs[i].remaining;
        if (burst->pieces[i].phase != EMPTY && !advance_piece(burst,&burst->pieces[i],run)) return 0;
    }
    return 1;
}

static int puff_card(const BfmHeroSmokeMaterial *material, const Puff *puff,
                     const BfmBedroomView *view, BfmForestDustCard *card)
{
    unsigned age = BFM_HERO_SMOKE_CELLS-puff->remaining;
    const BfmHeroSmokeFrame *frame = bfm_hero_smoke_frame(material,0,age);
    int xy[3];
    if (!frame || !bfm_bedroom_project(view,puff->xyz,xy) || xy[2]/4+1 >= 4096) return 0;
    int64_t ratio = ((int64_t)view->focal*65536+xy[2]/2)/xy[2];
    int scale = 3*(int)(ratio > 131071 ? 131071 : ratio)/16;
    int x = xy[0]+down((int64_t)material->left*scale,4096);
    int y = xy[1]+down((int64_t)material->top*scale,4096);
    int w = material->width*scale/4096, h = material->height*scale/4096;
    if (!w || !h || x >= 320 || y >= 240 || x+w <= 0 || y+h <= 0) return 0;
    *card = (BfmForestDustCard){.bucket=(unsigned)(xy[2]/4+1),
        .material={.count=4,.page=frame->page,.clut=frame->palette,.textured=1,.translucent=1}};
    for (unsigned i = 0; i < 4; ++i)
        card->vertices[i] = (BfmForestRasterVertex){x+(i&1 ? w : 0), y+(i&2 ? h : 0),
            frame->uv[i][0],frame->uv[i][1],0x808080};
    return 1;
}

static int add_piece(const BfmWoodsCasterBurst *burst, const Piece *p, BfmBedroomFigure *figure,
                     const BfmBedroomView *view)
{
    int16_t position[3] = {signed_word(p->xyz[0]+p->sway[0]),p->xyz[1],
                           signed_word(p->xyz[2]+p->sway[1])};
    BfmForestTransform root;
    bfm_forest_actor_pitch_yaw((unsigned)p->pitch,p->yaw,position,&root);
    if (p->phase == DISSOLVING && p->timer < 8) {
        int width = sine(p->timer*128);
        /* The renderer's signed scale field wraps near the end. */
        int height = signed_word(4096+8*sine(1024+p->timer*128));
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned col = 0; col < 3; ++col)
                root.rotation[row][col] = signed_word(down((int64_t)root.rotation[row][col]*
                                                           (col == 1 ? height : width),4096));
    }
    BfmForestActor actor = {.root=&root,.pose=&burst->identity,.mesh=burst->body,
                            .visible_parts=1u<<burst->parts[p->kind]};
    if (p->kind == 2) {
        actor.mesh = burst->third_mesh;
        actor.visible_parts = 1;
    }
    return bfm_bedroom_figure_add_actor(figure,view,&actor,NULL,NULL,0);
}

int bfm_woods_caster_burst_add(const BfmWoodsCasterBurst *burst, BfmBedroomFigure *figure,
                               const BfmBedroomView *view)
{
    if (!burst || !figure || !view || view->focal <= 0 || view->focal > UINT16_MAX) return 0;
    for (unsigned i = 0; i < ACTOR_SLOTS; ++i) {
        const Puff *p = &burst->puffs[i];
        BfmForestDustCard card;
        if (p->remaining && puff_card(&burst->smoke,p,view,&card) &&
            !bfm_bedroom_figure_add_fusion(figure,view,NULL,&card,1)) return 0;
        if (burst->pieces[i].phase != EMPTY && !add_piece(burst,&burst->pieces[i],figure,view))
            return 0;
    }
    return 1;
}
