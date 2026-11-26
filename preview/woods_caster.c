/* SPDX-License-Identifier: MIT */
#include "woods_caster.h"
#include "bedroom_visitor.h"
#include "field_waypoint.h"
#include "forest_guard_contact.h"
#include "forest_pose.h"
#include "village_ground.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t scene_origin = UINT32_C(0x80128158);
static const uint32_t woods_room = UINT32_C(0x8018ea38);

/* The movement and hit-reaction clips reference packed transforms in the user's scene. */
typedef enum { REST_POSE, RUSH_POSE, APPROACH_POSE, CAST_POSE, HIT_POSE, CLIPS } ClipId;
static const uint32_t clips[CLIPS] = {
    UINT32_C(0x801c7aa4), UINT32_C(0x801c7b9c),
    UINT32_C(0x801c7c74), UINT32_C(0x801c7d4c), UINT32_C(0x801c8024)
};

typedef struct {
    const uint8_t *rows;
    unsigned length;
    int loop;
} Clip;

typedef enum {
    OUTSIDE, WAIT_FOR_VIEW, APPEAR, SCAN, WALK, RUSH, VANISH,
    HIDDEN, EMERGE, APPROACH, CAST, ABSORBED, RELEASING, HIT_REACTION
} Phase;

typedef struct {
    int16_t home[3], xyz[3];
    int home_yaw, yaw, fraction_x, fraction_z;
    Phase phase;
    unsigned remaining, key;
    ClipId clip;
    int bound, tether_width;
    unsigned meter;
} Caster;

struct BfmWoodsCaster {
    const uint8_t *scene;
    size_t bytes;
    Caster actor[BFM_WOODS_CASTERS];
    Clip motion[CLIPS];
    BfmForestGuardBox body;
    BfmBedroomVisitor *model;
    BfmVillageGround *ground;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (!scene || address < scene_origin || address-scene_origin > bytes ||
        length > bytes-(address-scene_origin)) return NULL;
    return scene+(address-scene_origin);
}

static int read_clip(Clip *clip, const uint8_t *scene, size_t bytes, uint32_t address)
{
    clip->rows = mounted(scene,bytes,address,8);
    if (!clip->rows) return 0;
    for (unsigned i = 0; i < 256; ++i) {
        const uint8_t *row = mounted(scene,bytes,address+8*i,8);
        if (!row) return 0;
        uint32_t pose = u32le(row), span = u32le(row+4);
        if (!pose) {
            if (!i || (span != 0xffff && span != 0xfffe)) return 0;
            clip->length = i;
            clip->loop = span == 0xffff;
            return 1;
        }
        if ((pose >> 24) != 0x81 || span != 1 ||
            !mounted(scene,bytes,pose & ~UINT32_C(0x01000000),4*8)) return 0;
    }
    return 0;
}

static int read_placements(BfmWoodsCaster *casters, const uint8_t *scene, size_t bytes)
{
    const uint8_t *room = mounted(scene,bytes,woods_room,0xbc);
    if (!room) return 0;
    uint32_t first = u32le(room+0xb8);
    unsigned count = 0;
    /* A zero class ends this room's dynamic placement list. */
    const uint8_t *list = mounted(scene,bytes,first,20);
    if (!list) return 0;
    size_t available = bytes-(first-scene_origin);
    for (size_t at = 0; available-at >= 20; at += 20) {
        const uint8_t *row = list+at;
        unsigned type = u16le(row+6);
        if (!type) return count == BFM_WOODS_CASTERS;
        if (type != 108 || u16le(row+8) != 1) continue;
        if (count == BFM_WOODS_CASTERS) return 0;
        Caster *actor = &casters->actor[count++];
        for (unsigned k = 0; k < 3; ++k) actor->home[k] = s16le(row+2*k);
        actor->home_yaw = u16le(row+12) & 4095;
    }
    return 0;
}

BfmWoodsCaster *bfm_woods_caster_load(const uint8_t *scene, size_t bytes)
{
    const uint8_t *profile = mounted(scene,bytes,UINT32_C(0x801c473c),46);
    if (!profile || u16le(profile) != 64 || !u16le(profile+2) || u16le(profile+44) != 13) return NULL;
    const uint8_t *model = mounted(scene,bytes,u32le(profile+4),5*4);
    if (!model || u32le(model+16)) return NULL;
    for (unsigned i = 0; i < 4; ++i)
        if (!mounted(scene,bytes,u32le(model+4*i),4)) return NULL;
    const uint8_t *shape = mounted(scene,bytes,u32le(profile+8),16);
    if (!shape || u32le(shape)) return NULL;
    BfmWoodsCaster *casters = calloc(1,sizeof(*casters));
    if (!casters) return NULL;
    casters->scene = scene;
    casters->bytes = bytes;
    for (unsigned k = 0; k < 3; ++k) {
        casters->body.low[k] = s16le(shape+4+k*4);
        casters->body.high[k] = s16le(shape+6+k*4);
        /* The actor origin lies in its body box. */
        if (casters->body.low[k] > 0 || casters->body.high[k] < 0) goto fail;
    }
    if (!read_placements(casters,scene,bytes)) goto fail;
    for (unsigned k = 0; k < CLIPS; ++k)
        if (!read_clip(&casters->motion[k],scene,bytes,clips[k])) goto fail;

    uint32_t table = u32le(profile+4), models[BFM_WOODS_CASTERS] = {table,table};
    casters->model = bfm_bedroom_visitor_load(scene,bytes,models,BFM_WOODS_CASTERS,UINT32_C(0x80189128));
    casters->ground = bfm_village_ground_open(scene,bytes,woods_room);
    if (casters->model && casters->ground) return casters;
fail:
    bfm_woods_caster_free(casters);
    return NULL;
}

void bfm_woods_caster_free(BfmWoodsCaster *casters)
{
    if (!casters) return;
    bfm_bedroom_visitor_free(casters->model);
    bfm_village_ground_free(casters->ground);
    free(casters);
}

static int64_t distance2(const int16_t a[3], const int16_t b[3])
{
    int64_t x = a[0]-b[0], y = a[1]-b[1], z = a[2]-b[2];
    return x*x+y*y+z*z;
}

static void enter(Caster *actor, Phase phase, unsigned duration, ClipId clip)
{
    actor->phase = phase;
    actor->remaining = duration;
    if (actor->clip != clip || phase == SCAN) {
        actor->clip = clip;
        actor->key = 0;
    }
}

static void place_home(Caster *actor)
{
    memcpy(actor->xyz,actor->home,sizeof(actor->xyz));
    actor->yaw = actor->home_yaw;
    actor->fraction_x = actor->fraction_z = 0;
}

static int yaw_error(const Caster *actor, const BfmBedroomRoot *player)
{
    int bearing = bfm_waypoint_heading(actor->xyz[0]-player->xyz[0],actor->xyz[2]-player->xyz[2]);
    return ((bearing-actor->yaw+2048) & 4095)-2048;
}

static void track(Caster *actor, int error)
{
    int turn = error/8;
    if (!turn) turn = (error > 0)-(error < 0);
    actor->yaw = (actor->yaw+turn) & 4095;
}

static int advance_coord(int current, int direction, int speed_half, int *fraction, int16_t *out)
{
    int q = *fraction-direction*speed_half;
    int whole = q/8192-(q%8192 < 0);
    if (current+whole < INT16_MIN || current+whole > INT16_MAX) return 0;
    *fraction = q-whole*8192;
    *out = (int16_t)(current+whole);
    return 1;
}

static int walk(const BfmWoodsCaster *casters, Caster *actor, int speed_half)
{
    int fx = actor->fraction_x, fz = actor->fraction_z;
    int16_t end[3] = {actor->xyz[0],actor->xyz[1],actor->xyz[2]};
    BfmForestTransform root;
    bfm_forest_actor_root((unsigned)actor->yaw,actor->xyz,&root);
    if (!advance_coord(end[0],root.rotation[0][2],speed_half,&fx,&end[0]) ||
        !advance_coord(end[2],root.rotation[2][2],speed_half,&fz,&end[2])) return 0;
    int x = end[0], z = end[2];
    uint16_t wall = 0;
    BfmForestContact floor;
    if (bfm_village_ground_wall_step(casters->ground,actor->xyz,end,&floor,&wall) != BFM_VILLAGE_GROUND_FOOTING ||
        wall || end[0] != x || end[2] != z) return 0;
    memcpy(actor->xyz,end,sizeof(end));
    actor->fraction_x = fx;
    actor->fraction_z = fz;
    return 1;
}

static void relocate(const BfmWoodsCaster *casters, Caster *actor, const BfmBedroomRoot *player)
{
    int64_t dx = actor->home[0]-actor->xyz[0], dz = actor->home[2]-actor->xyz[2];
    if (dx*dx+dz*dz >= 1280*1280) {
        place_home(actor);
        return;
    }
    BfmForestTransform root;
    bfm_forest_actor_root(player->yaw,player->xyz,&root);
    for (int distance = 160; distance; distance /= 2) {
        int rx = 0, rz = 0;
        int16_t point[3] = {player->xyz[0],player->xyz[1],player->xyz[2]};
        if (!advance_coord(point[0],root.rotation[0][2],2*distance,&rx,&point[0]) ||
            !advance_coord(point[2],root.rotation[2][2],2*distance,&rz,&point[2])) continue;
        int x = point[0], z = point[2];
        uint16_t wall = 0;
        BfmForestContact floor;
        if (bfm_village_ground_wall_step(casters->ground,player->xyz,point,&floor,&wall) == BFM_VILLAGE_GROUND_FOOTING &&
            !wall && point[0] == x && point[2] == z) {
            memcpy(actor->xyz,point,sizeof(point));
            actor->fraction_x = actor->fraction_z = 0;
            break;
        }
    }
    actor->yaw = (actor->yaw+yaw_error(actor,player)) & 4095;
}

static int in_view(const BfmBedroomView *view, const int16_t position[3])
{
    int screen[3];
    return bfm_bedroom_project(view,position,screen) &&
           screen[0] >= 20 && screen[0] <= 300 && screen[1] >= 20 && screen[1] <= 220;
}

static void next_pose(const BfmWoodsCaster *casters, Caster *actor)
{
    const Clip *clip = &casters->motion[actor->clip];
    if (actor->key+1 < clip->length) ++actor->key;
    else if (clip->loop) actor->key = 0;
}

static void update_caster(BfmWoodsCaster *casters, Caster *actor, const BfmBedroomView *view,
                          const BfmBedroomRoot *player)
{
    if (actor->bound) {
        /* Fusion keeps the contact key still while the visible body narrows. */
        int desired = (144-(int)actor->meter)*4096/144;
        if (actor->tether_width < 0 || !actor->meter) actor->tether_width = 4096;
        actor->tether_width += (desired-actor->tether_width)/4;
        return;
    }
    if (actor->phase == OUTSIDE) {
        /* Dynamic actors mount as the player's admission band crosses a placement. */
        int64_t range = distance2(actor->home,player->xyz);
        if (range > 1536*1536 && range < 1600*1600) {
            place_home(actor);
            enter(actor,WAIT_FOR_VIEW,0,REST_POSE);
        }
        return;
    }
    int64_t range = distance2(actor->xyz,player->xyz);
    if (range >= 2048*2048) {
        enter(actor,OUTSIDE,0,REST_POSE);
        actor->key = 0;
        return;
    }
    if (actor->phase == ABSORBED) {
        if (actor->remaining) --actor->remaining;
        if (!actor->remaining && range >= 320*320) enter(actor,APPEAR,8,REST_POSE);
        return;
    }
    if (actor->phase == RELEASING) {
        actor->tether_width += (4096-actor->tether_width)/4;
        if (!--actor->remaining) {
            actor->tether_width = 4096;
            enter(actor,HIT_REACTION,casters->motion[HIT_POSE].length,HIT_POSE);
        }
        return;
    }
    if (actor->phase == HIT_REACTION) {
        if (--actor->remaining) next_pose(casters,actor);
        else enter(actor,SCAN,32,REST_POSE);
        return;
    }
    if (actor->phase == WAIT_FOR_VIEW) {
        if (in_view(view,actor->xyz)) enter(actor,APPEAR,8,REST_POSE);
        return;
    }
    if (actor->phase == HIDDEN) {
        if (!--actor->remaining) {
            relocate(casters,actor,player);
            enter(actor,EMERGE,8,APPROACH_POSE);
        }
        return;
    }
    next_pose(casters,actor);
    int error = yaw_error(actor,player);
    switch (actor->phase) {
    case APPEAR:
        if (!--actor->remaining) enter(actor,SCAN,32,REST_POSE);
        break;
    case SCAN:
    case WALK:
        if (in_view(view,actor->xyz) && (range <= 8192 || (range <= 384*384 && abs(error) < 1024))) {
            enter(actor,RUSH,36,RUSH_POSE);
        } else if (actor->phase == SCAN) {
            actor->yaw = (actor->yaw+64) & 4095;
            if (!--actor->remaining) enter(actor,WALK,0,REST_POSE);
        } else if (!walk(casters,actor,3)) {
            enter(actor,SCAN,32,REST_POSE);
        }
        break;
    case RUSH:
        track(actor,error);
        if (range <= 8192 || (actor->remaining <= 24 && !walk(casters,actor,8)) || !--actor->remaining)
            enter(actor,VANISH,8,RUSH_POSE);
        break;
    case VANISH:
        if (!--actor->remaining) enter(actor,HIDDEN,30,RUSH_POSE);
        break;
    case EMERGE:
        if (!--actor->remaining) enter(actor,APPROACH,26,APPROACH_POSE);
        break;
    case APPROACH:
        track(actor,error);
        if (range <= 12288 || (--actor->remaining == 0 && range <= 28672)) {
            enter(actor,CAST,90,CAST_POSE);
        } else if (!actor->remaining || !walk(casters,actor,8)) {
            enter(actor,SCAN,32,REST_POSE);
        }
        break;
    case CAST:
        if (actor->remaining >= 22) track(actor,error);
        if (range > 288*288 || !--actor->remaining) enter(actor,SCAN,32,REST_POSE);
        break;
    default:
        break;
    }
}

int bfm_woods_caster_step(BfmWoodsCaster *casters, const BfmBedroomView *view,
                          const BfmBedroomRoot *player)
{
    if (!casters || !view || !player) return 0;
    for (unsigned i = 0; i < BFM_WOODS_CASTERS; ++i)
        update_caster(casters,&casters->actor[i],view,player);
    return 1;
}

static int visible(const Caster *actor)
{
    return actor->phase != OUTSIDE && actor->phase != WAIT_FOR_VIEW &&
           actor->phase != HIDDEN && actor->phase != ABSORBED;
}

int bfm_woods_caster_add(const BfmWoodsCaster *casters, BfmBedroomFigure *figure,
                         const BfmBedroomView *view)
{
    if (!casters || !figure || !view) return 0;
    for (unsigned i = 0; i < BFM_WOODS_CASTERS; ++i) {
        const Caster *actor = &casters->actor[i];
        if (!visible(actor) && actor->phase != HIDDEN) continue;
        unsigned width = 4096, height = 4096;
        if ((actor->bound && actor->tether_width >= 0) || actor->phase == RELEASING) {
            width = (unsigned)actor->tether_width;
        } else if (actor->phase == HIDDEN) {
            width = 0;  /* Its independent floor shadow stays visible. */
        } else if (actor->phase == APPEAR || actor->phase == EMERGE || actor->phase == VANISH) {
            unsigned tick = actor->remaining;
            if (actor->phase != VANISH && tick == 8) tick = 7;
            BfmForestTransform phase;
            bfm_forest_actor_root(tick*128,actor->xyz,&phase);
            unsigned sine = (unsigned)phase.rotation[0][2];
            unsigned cosine = (unsigned)phase.rotation[0][0];
            width = actor->phase == VANISH ? sine : cosine;
            height += 2*(actor->phase == VANISH ? cosine : sine);
        }
        const Clip *clip = &casters->motion[actor->clip];
        uint32_t pose = u32le(clip->rows+actor->key*8) & ~UINT32_C(0x01000000);
        int position[3] = {actor->xyz[0],actor->xyz[1],actor->xyz[2]};
        BfmBedroomRoot ground = {0};
        /* The shadow child projects from Y-20 to Y+748 independently of the body. */
        memcpy(ground.xyz,actor->xyz,sizeof(ground.xyz));
        if (!bfm_bedroom_travel_air_shadow(casters->scene,casters->bytes,woods_room,&ground)) return 0;
        if (!bfm_bedroom_visitor_add_scaled(casters->model,figure,view,i,position,actor->yaw,
                                           pose,width,height,&ground))
            return 0;
    }
    return 1;
}

static const Caster *active(const BfmWoodsCaster *casters, int target)
{
    if (!casters || target < 1 || target > BFM_WOODS_CASTERS) return NULL;
    const Caster *actor = &casters->actor[target-1];
    return visible(actor) && actor->phase != VANISH && actor->phase != RELEASING &&
           actor->phase != HIT_REACTION ? actor : NULL;
}

int bfm_woods_caster_root(const BfmWoodsCaster *casters, int target, int16_t root[3])
{
    const Caster *actor = active(casters,target);
    if (!actor || !root) return 0;
    memcpy(root,actor->xyz,sizeof(actor->xyz));
    return 1;
}

int bfm_woods_caster_target(const BfmWoodsCaster *casters, int target, int16_t center[3])
{
    const Caster *actor = active(casters,target);
    if (!actor || !center) return 0;
    BfmForestTransform root;
    bfm_forest_actor_root((unsigned)actor->yaw,actor->xyz,&root);
    /* The verified root is inside the body; use the same center calculation as a strike. */
    return bfm_forest_guard_contact(&casters->body,&root,actor->xyz,actor->xyz,center);
}

int bfm_woods_caster_contact(const BfmWoodsCaster *casters, const int16_t start[3],
                             const int16_t end[3], int16_t contact[3], int16_t center[3])
{
    if (!casters || !start || !end || !contact || !center) return 0;
    int nearest = 0;
    int64_t best = INT64_MAX;
    int16_t best_hit[3], best_center[3];
    for (int target = 1; target <= BFM_WOODS_CASTERS; ++target) {
        const Caster *actor = active(casters,target);
        if (!actor || actor->bound) continue;
        BfmForestTransform root;
        bfm_forest_actor_root((unsigned)actor->yaw,actor->xyz,&root);
        int16_t midpoint[3], hit[3];
        if (!bfm_forest_guard_contact_point(&casters->body,&root,start,end,midpoint,hit,NULL)) continue;
        int64_t distance = distance2(start,hit);
        if (distance < best) {
            nearest = target;
            best = distance;
            memcpy(best_hit,hit,sizeof(hit));
            memcpy(best_center,midpoint,sizeof(midpoint));
        }
    }
    if (nearest) {
        memcpy(contact,best_hit,sizeof(best_hit));
        memcpy(center,best_center,sizeof(best_center));
    }
    return nearest;
}

int bfm_woods_caster_latch(BfmWoodsCaster *casters, int target)
{
    const Caster *actor = active(casters,target);
    if (!actor || actor->bound) return 0;
    Caster *tethered = &casters->actor[target-1];
    tethered->bound = 1;
    tethered->tether_width = -1;  /* Keep the contact frame until the first tether update. */
    tethered->meter = 0;
    return 1;
}

void bfm_woods_caster_meter(BfmWoodsCaster *casters, int target, unsigned meter)
{
    if (!casters || target < 1 || target > BFM_WOODS_CASTERS) return;
    Caster *actor = &casters->actor[target-1];
    if (actor->bound) actor->meter = meter > 128 ? 128 : meter;
}

void bfm_woods_caster_release(BfmWoodsCaster *casters, int target)
{
    if (!casters || target < 1 || target > BFM_WOODS_CASTERS) return;
    Caster *actor = &casters->actor[target-1];
    if (!actor->bound) return;
    actor->bound = 0;
    if (actor->tether_width < 0) actor->tether_width = 4096;
    enter(actor,RELEASING,6,actor->clip);
}

int bfm_woods_caster_absorb(BfmWoodsCaster *casters, int target)
{
    if (!active(casters,target)) return 0;
    Caster *actor = &casters->actor[target-1];
    if (!actor->bound) return 0;
    actor->bound = 0;
    /* Absorption rearms this placement at home. Its body and shadow stay
     * absent through the cooldown and until the player clears the bank. */
    place_home(actor);
    actor->yaw = 0;
    actor->key = 0;
    enter(actor,ABSORBED,64,REST_POSE);
    return 1;
}
