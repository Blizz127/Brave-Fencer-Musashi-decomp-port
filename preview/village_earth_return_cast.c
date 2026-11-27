/* SPDX-License-Identifier: MIT */
#include "village_earth_return_cast.h"
#include "bedroom_visitor.h"
#include "field_waypoint.h"
#include "forest_pose.h"
#include "village_ground.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { Q16 = 65536, TURN = 4096 };
static const uint32_t upper = UINT32_C(0x8018c794);
enum { JON, DOG, PEOPLE };
typedef enum {
    JON_IDLE, JON_WALK, JON_SPEAK, DOG_RUN, DOG_EASE, DOG_WAIT,
    DOG_LOOK, DOG_SNIFF, DOG_CALLED, CLIPS
} ClipId;
typedef struct { const uint8_t *keys; unsigned count; } Clip;
typedef struct {
    BfmBedroomRoot root;
    int32_t frac_x, frac_z;
    ClipId clip;
    unsigned key;
    int visible;
} Member;
typedef enum { STILL, APPROACH, FACE, STRIDE } JonMotion;
typedef enum { WAIT_ENTRANCE, RUN_IN, EASE_IN, PAUSE, WAIT_CALL, TURN_TO_JON,
               FOLLOW, RUN_OUT, GONE } DogMotion;

typedef struct {
    BfmEarthCastCommand command;
    uint32_t argument;
    int16_t target[3];
} Cue;

struct BfmVillageEarthReturnCast {
    BfmBedroomVisitor *bodies;
    BfmVillageGround *floor;
    Clip clips[CLIPS];
    Member member[PEOPLE];
    JonMotion walk;
    int16_t walk_to[3], dog_marker[3];
    unsigned goal_yaw, turn_count, stride_count;
    int32_t stride;
    int farewell_look, farewell_done;
    DogMotion dog_motion;
    unsigned dog_age, dog_signal, dog_delay, approach_end, settle_steps;
    int32_t dog_run, dog_ease;
    Cue pending[12];
    unsigned cues;
};

static const uint8_t *span(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || count > bytes || address-base > bytes-count) return NULL;
    return scene + (address-base);
}

static int load_clip(Clip *clip, const uint8_t *scene, size_t bytes, uint32_t address, unsigned parts)
{
    /* Each body key belongs to this field, with one field of playback. */
    for (unsigned i = 0; i < 64; ++i) {
        const uint8_t *key = span(scene,bytes,address+i*8,8);
        if (!key) return 0;
        uint32_t pose = u32le(key), length = u32le(key+4);
        if (!pose && length == 0xffffu) {
            clip->keys = key-i*8;
            clip->count = i;
            return i != 0;
        }
        if ((pose >> 24) != 0x81 || length != 1 ||
            !span(scene,bytes,pose & ~UINT32_C(0x01000000), parts*8u)) return 0;
    }
    return 0;
}

void bfm_village_earth_return_cast_free(BfmVillageEarthReturnCast *c)
{
    if (c) {
        bfm_bedroom_visitor_free(c->bodies);
        bfm_village_ground_free(c->floor);
        free(c);
    }
}

BfmVillageEarthReturnCast *bfm_village_earth_return_cast_open(const uint8_t *scene, size_t bytes)
{
    const uint8_t *jon = span(scene,bytes,UINT32_C(0x80189a40),24);
    const uint8_t *dog = span(scene,bytes,UINT32_C(0x80189cd8),28);
    const uint8_t *dog_model = span(scene,bytes,UINT32_C(0x801892c8),8);
    if (!jon || !dog || !dog_model) return NULL;
    BfmVillageEarthReturnCast *c = calloc(1,sizeof *c);
    if (!c) return NULL;
    uint32_t tables[PEOPLE] = {u32le(jon+8), u32le(dog_model+4)};
    uint32_t programs[CLIPS] = {
        u32le(jon+12), u32le(jon+16), u32le(jon+20),
        UINT32_C(0x801c887c), UINT32_C(0x801c855c), UINT32_C(0x80189ca8),
        UINT32_C(0x801c8634), UINT32_C(0x801c868c), UINT32_C(0x80189c00)
    };
    c->bodies = bfm_bedroom_visitor_load(scene,bytes,tables,PEOPLE,UINT32_C(0x80186e14));
    c->floor = bfm_village_ground_open(scene,bytes,upper);
    if (!c->bodies || !c->floor) goto fail;
    for (unsigned i = 0; i < CLIPS; ++i)
        if (!load_clip(&c->clips[i],scene,bytes,programs[i],i <= JON_SPEAK ? 10 : 12)) goto fail;
    Member *j = &c->member[JON], *d = &c->member[DOG];
    for (unsigned axis = 0; axis < 3; ++axis) {
        j->root.xyz[axis] = s16le(jon+2*axis);
        c->dog_marker[axis] = s16le(dog+20+2*axis);
    }
    j->root.yaw = u16le(jon+6) & 4095u;
    if (!bfm_bedroom_travel_air_shadow(scene,bytes,upper,&j->root)) goto fail;
    d->root = j->root; /* The child registers at its parent's actual spawn. */
    d->root.yaw = 0;
    j->clip = JON_WALK;
    d->clip = DOG_RUN;
    j->visible = d->visible = 1;
    c->dog_delay = u16le(dog);
    c->approach_end = u16le(dog+6);
    c->dog_ease = (int32_t)u32le(dog+8);
    c->dog_run = (int32_t)u32le(dog+12);
    c->settle_steps = u32le(dog+16);
    if (!c->dog_delay || c->approach_end <= c->dog_delay || c->approach_end > 1024 ||
        !c->settle_steps || c->settle_steps > 1024 ||
        c->dog_run >= 0 || c->dog_run < -32*Q16 || c->dog_ease >= 0 || c->dog_ease < -32*Q16) goto fail;
    return c;
fail:
    bfm_village_earth_return_cast_free(c);
    return NULL;
}

static unsigned bearing(const Member *who, const int16_t point[3])
{
    int x = who->root.xyz[0]-point[0], z = who->root.xyz[2]-point[2];
    return x || z ? (unsigned)bfm_waypoint_heading(x,z) : 3072u;
}

static void face(Member *who, unsigned goal, int divisor)
{
    int difference = (int)goal - (int)who->root.yaw;
    if (difference > 2048) difference -= TURN;
    if (difference < -2048) difference += TURN;
    int step = difference/divisor;
    if (!step) step = (difference > 0) - (difference < 0);
    who->root.yaw = (who->root.yaw+step) & 4095u;
}

static int whole(int64_t value, int32_t *fraction)
{
    int64_t integer = value/Q16 - (value%Q16 < 0);
    *fraction = (int32_t)(value-integer*Q16);
    return (int)integer;
}

static int move(BfmVillageEarthReturnCast *c, Member *who, int32_t speed)
{
    BfmForestTransform heading;
    bfm_forest_actor_root(who->root.yaw,who->root.xyz,&heading);
    int32_t fx, fz;
    int x = who->root.xyz[0] + whole(who->frac_x + ((int64_t)speed*heading.rotation[0][2])/4096,&fx);
    int z = who->root.xyz[2] + whole(who->frac_z + ((int64_t)speed*heading.rotation[2][2])/4096,&fz);
    if (x < INT16_MIN || x > INT16_MAX || z < INT16_MIN || z > INT16_MAX) return 0;
    int16_t next[3] = {(int16_t)x, who->root.xyz[1], (int16_t)z};
    BfmForestContact support;
    BfmVillageGroundResult ground = bfm_village_ground_step(c->floor,who->root.xyz,next,&support);
    if (ground != BFM_VILLAGE_GROUND_BLOCKED) {
        who->root.shadow_visible = ground == BFM_VILLAGE_GROUND_FOOTING;
        /* A successful floor probe returns integer coordinates for both actors. */
        who->frac_x = who->root.shadow_visible ? 0 : fx;
        who->frac_z = who->root.shadow_visible ? 0 : fz;
        memcpy(who->root.xyz,next,sizeof next);
        if (who->root.shadow_visible) {
            memcpy(who->root.shadow_xyz,next,sizeof next);
            memcpy(who->root.shadow_normal,support.normal,sizeof who->root.shadow_normal);
        }
    }
    return 1;
}

static int64_t distance2(const Member *a, const int16_t point[3])
{
    int64_t x = a->root.xyz[0] - point[0], z = a->root.xyz[2] - point[2];
    return x*x + z*z;
}

static void choose(Member *who, ClipId clip)
{
    who->clip = clip;
    who->key = 0;
}

static int step_jon(BfmVillageEarthReturnCast *c)
{
    Member *j = &c->member[JON];
    if (c->walk == APPROACH) {
        unsigned yaw = bearing(j,c->walk_to);
        if (c->turn_count) { face(j,yaw,4); --c->turn_count; }
        else {
            j->root.yaw = yaw;
            if (!move(c,j,-6*Q16)) return 0;
            if (distance2(j,c->walk_to) <= 256) c->walk = STILL;
        }
    } else if (c->walk == FACE) {
        face(j,c->goal_yaw,6);
        if (!--c->turn_count) { j->root.yaw = c->goal_yaw; c->walk = STILL; }
    } else if (c->walk == STRIDE) {
        if (!move(c,j,c->stride)) return 0;
        if (!--c->stride_count) c->walk = STILL;
    }
    return 1;
}

static int step_dog(BfmVillageEarthReturnCast *c, BfmGameplayRun *run)
{
    Member *d = &c->member[DOG];
    const int16_t *jon = c->member[JON].root.xyz;
    switch (c->dog_motion) {
    case WAIT_ENTRANCE:
        if (++c->dog_age > c->dog_delay) c->dog_motion = RUN_IN;
        break;
    case RUN_IN:
        d->root.yaw = bearing(d,c->dog_marker);
        if (!move(c,d,c->dog_run)) return 0;
        if (++c->dog_age >= c->approach_end) {
            c->dog_age = 0;
            choose(d,DOG_EASE);
            c->dog_motion = EASE_IN;
        }
        break;
    case EASE_IN:
        if (!move(c,d,c->dog_ease)) return 0;
        if (++c->dog_age >= c->settle_steps) {
            choose(d,DOG_WAIT);
            c->dog_motion = PAUSE;
            c->dog_age = 5;
        }
        break;
    case PAUSE:
        if (!--c->dog_age) c->dog_motion = WAIT_CALL;
        break;
    case WAIT_CALL:
        if (!c->dog_age || !--c->dog_age) {
            if (!run) return 0;
            c->dog_age = 180 + (bfm_gameplay_random(run) & 31u);
            choose(d,(bfm_gameplay_random(run) & 1u) ? DOG_LOOK : DOG_SNIFF);
        }
        if (c->dog_signal == 1) {
            choose(d,DOG_CALLED);
            c->dog_age = 6;
            c->dog_motion = TURN_TO_JON;
        }
        break;
    case TURN_TO_JON:
        if (c->dog_age && --c->dog_age == 1) choose(d,DOG_EASE);
        face(d,bearing(d,jon),8);
        if (distance2(d,jon) >= 0x7900) {
            choose(d,DOG_RUN);
            c->dog_motion = FOLLOW;
        }
        break;
    case FOLLOW:
    case RUN_OUT:
        face(d,bearing(d,jon),8);
        if (!move(c,d,-6*Q16)) return 0;
        if (c->dog_motion == FOLLOW && c->dog_signal == 2) {
            c->dog_age = 16;
            c->dog_motion = RUN_OUT;
        } else if (c->dog_motion == RUN_OUT && !--c->dog_age) {
            d->visible = 0;
            c->dog_motion = GONE;
        }
        break;
    case GONE: break;
    }
    return 1;
}

int bfm_village_earth_return_cast_command(BfmVillageEarthReturnCast *c, BfmEarthCastCommand command,
                                          uint32_t arg, const int16_t hero[3])
{
    if (!c || (unsigned)command > BFM_EARTH_CAST_RELEASE_DOG || c->cues >= sizeof c->pending/sizeof c->pending[0] ||
        (command == BFM_EARTH_CAST_FACE_HERO && !hero) ||
        (command == BFM_EARTH_CAST_WALK && !(arg >> 16))) return 0;
    Cue *q = &c->pending[c->cues++];
    q->command = command;
    q->argument = arg;
    if (hero) memcpy(q->target,hero,sizeof q->target);
    return 1;
}

static void dispatch(BfmVillageEarthReturnCast *c, const Cue *q)
{
    Member *j = &c->member[JON];
    uint32_t arg = q->argument;
    switch (q->command) {
    case BFM_EARTH_CAST_APPROACH:
        c->walk_to[0] = (int16_t)arg; c->walk_to[2] = (int16_t)(arg >> 16);
        c->walk = APPROACH; c->turn_count = 16;
        break;
    case BFM_EARTH_CAST_IDLE: choose(j,JON_IDLE); break;
    case BFM_EARTH_CAST_FACE_HERO:
        c->goal_yaw = bearing(j,q->target); c->turn_count = 30; c->walk = FACE;
        break;
    case BFM_EARTH_CAST_SPEAK: choose(j,JON_SPEAK); break;
    case BFM_EARTH_CAST_CALL_DOG: c->dog_signal = 1; break;
    case BFM_EARTH_CAST_FINISH_LOOK: c->farewell_done = 1; break;
    case BFM_EARTH_CAST_WALK_POSE: choose(j,JON_WALK); break;
    case BFM_EARTH_CAST_TURN:
        c->goal_yaw = (j->root.yaw + (int16_t)arg) & 4095u; c->turn_count = 30; c->walk = FACE;
        break;
    case BFM_EARTH_CAST_WALK:
        c->stride = (int16_t)arg*256; c->stride_count = arg >> 16; c->walk = STRIDE;
        break;
    case BFM_EARTH_CAST_HIDE_JON: j->visible = 0; break;
    case BFM_EARTH_CAST_RELEASE_DOG: c->dog_signal = 2; break;
    }
}

int bfm_village_earth_return_cast_step(BfmVillageEarthReturnCast *c, BfmGameplayRun *run, int flag266)
{
    if (!c) return 0;
    int was_looking = c->farewell_look && !c->farewell_done;
    for (unsigned i = 0; i < PEOPLE; ++i) {
        Member *m = &c->member[i];
        m->key = (m->key+1) % c->clips[m->clip].count;
    }
    if (!step_jon(c)) return 0;
    for (unsigned i = 0; i < c->cues; ++i) dispatch(c,&c->pending[i]);
    c->cues = 0;
    if (was_looking) face(&c->member[JON],480,8);
    if (flag266) c->farewell_look = 1;
    return step_dog(c,run);
}

const int16_t *bfm_village_earth_return_cast_position(const BfmVillageEarthReturnCast *c, int dog)
{
    if (!c) return NULL;
    const Member *m = &c->member[dog ? DOG : JON];
    return m->visible ? m->root.xyz : NULL;
}

int bfm_village_earth_return_cast_add(const BfmVillageEarthReturnCast *c,
                                     BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!c) return 1;
    for (unsigned i = 0; i < PEOPLE; ++i) {
        const Member *who = &c->member[i];
        if (!who->visible) continue;
        int point[3] = {who->root.xyz[0], who->root.xyz[1], who->root.xyz[2]};
        uint32_t pose = u32le(c->clips[who->clip].keys+who->key*8) & ~UINT32_C(0x01000000);
        if (!bfm_bedroom_visitor_add_scaled(c->bodies,figure,view,i,point,who->root.yaw,pose,
                                            4096,4096,&who->root)) return 0;
    }
    return 1;
}
