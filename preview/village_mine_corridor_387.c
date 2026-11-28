/* SPDX-License-Identifier: MIT */
#include "village_mine_corridor_387.h"
#include "village_mine_view.h"
#include "field_waypoint.h"
#include "forest_guard_contact.h"
#include "forest_lift.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

enum Mine387Action { M387_START, M387_PATROL, M387_CHASE, M387_AIM, M387_LUNGE, M387_GONE };
enum Mine387Patrol { M387_STRAIGHT, M387_WANDER, M387_TURN_BACK };
enum Mine387Flight { M387_OUTWARD, M387_FALL, M387_SKID, M387_OFF_EDGE, M387_WALL_FALL };

typedef struct {
    int16_t xyz[3], yaw, pitch, scale_y, hit_yaw;
    int32_t position[3], velocity[3], gravity;
    enum Mine387Action action;
    enum Mine387Patrol patrol;
    enum Mine387Flight flight;
    int16_t turn_yaw, stop_y;
    unsigned phase, straight_left, timer;
    int dust, common_fall;
} Mine387Motion;

static void mine387_motion_start(Mine387Motion *m, const int16_t root[3], unsigned yaw)
{
    *m = (Mine387Motion){.yaw = (int16_t)yaw, .scale_y = 4096, .straight_left = 64};
    for (unsigned i = 0; i < 3; ++i) {
        m->xyz[i] = root[i];
        m->position[i] = (int32_t)root[i] * 65536;
    }
}

static int32_t mine387_down(int64_t value, int unit)
{
    return (int32_t)(value / unit - (value % unit < 0));
}

static int mine387_wave(unsigned angle)
{
    return (int)lround(4096.0 * sin((angle & 4095u) * 0.0015339807878856412));
}

/* The mesh spline accepts signed Euler angles. A negative argument negates
 * the sine from its positive table entry and keeps its cosine. */
static void mine387_aim_basis(const Mine387Motion *m, int16_t pitch, int16_t yaw,
                              BfmForestTransform *root)
{
    BfmForestTransform tilt, turn;
    bfm_forest_actor_root((unsigned)abs(pitch), m->xyz, &tilt);
    bfm_forest_actor_root((unsigned)abs(yaw), m->xyz, &turn);
    int sy = turn.rotation[0][2] * (yaw < 0 ? -1 : 1);
    int sx = tilt.rotation[0][2] * (pitch < 0 ? -1 : 1);
    int cx = tilt.rotation[0][0], cy = turn.rotation[0][0];
    *root = (BfmForestTransform){.rotation = {
        {cy,mine387_down(sy*sx,4096),mine387_down(sy*cx,4096)},
        {0,cx,-sx}, {-sy,mine387_down(cy*sx,4096),mine387_down(cy*cx,4096)}},
        .translation={m->xyz[0],m->xyz[1],m->xyz[2]}};
}

static void mine387_forward(Mine387Motion *m, int16_t pitch, int16_t yaw, int speed)
{
    BfmForestTransform aim;
    mine387_aim_basis(m,pitch,yaw,&aim);
    for (unsigned i = 0; i < 3; ++i)
        m->velocity[i] = mine387_down((int64_t)speed*aim.rotation[i][2],4096);
}

/* A normal stride sees the model's previous submitted basis. A ceiling
 * creature retains its horizontal compression after the landing. */
static void mine387_stride(Mine387Motion *m, const BfmForestTransform *submitted, int speed)
{
    for (unsigned i = 0; i < 3; ++i)
        m->velocity[i] = mine387_down((int64_t)speed * submitted->rotation[i][2],4096);
}

static int mine387_heading(const Mine387Motion *m, const int16_t target[3])
{
    return (bfm_waypoint_heading(m->xyz[2]-target[2], target[0]-m->xyz[0])-1024) & 4095;
}

static int mine387_yaw_error(int target, int yaw)
{
    int difference = (target & 4095) - (yaw & 4095);
    if (difference > 2048) difference -= 4096;
    if (difference < -2048) difference += 4096;
    return difference;
}

static int mine387_steer(int target, int yaw, int divisor)
{
    int delta = mine387_yaw_error(target,yaw);
    return abs(delta) < divisor ? (delta > 0) - (delta < 0) : delta / divisor;
}

static int64_t mine387_distance2(const Mine387Motion *m, const int16_t p[3], int y_offset)
{
    int64_t dx = (int)p[0]-m->xyz[0], dy = (int)p[1]+y_offset-m->xyz[1], dz = (int)p[2]-m->xyz[2];
    return dx*dx + dy*dy + dz*dz;
}

static int mine387_sees(const Mine387Motion *m, const int16_t aim[3])
{
    return aim && mine387_distance2(m,aim,0) <= 384*384 &&
           abs(mine387_yaw_error(mine387_heading(m,aim),m->yaw)) < 1024;
}

static unsigned mine387_travel_at(Mine387Motion *m, const BfmVillageGround *ground,
                                  unsigned mode, int vertical_offset)
{
    int16_t foot[3];
    m->velocity[1] += m->gravity;
    for (unsigned i = 0; i < 3; ++i) {
        m->position[i] += m->velocity[i];
        foot[i] = (int16_t)mine387_down(m->position[i],65536);
    }
    int16_t start[3];
    memcpy(start,m->xyz,sizeof start);
    start[1] = (int16_t)(start[1]+vertical_offset);
    foot[1] = (int16_t)(foot[1]+vertical_offset);
    uint16_t material = 0;
    unsigned contact = mode == 17 ? bfm_village_ground_rock_probe(ground,1,start,foot,&material) :
                                  bfm_village_ground_support_probe(ground,mode,start,foot,&material);
    if (mode == 0 && contact) {
        /* A walking actor confirms the projected point against the short
         * stationary foot probe. Only its Y replaces the swept endpoint. */
        int16_t below[3];
        memcpy(below,foot,sizeof below);
        uint16_t below_material;
        unsigned support = bfm_village_ground_support_probe(ground,2,foot,below,&below_material);
        if (support & 0x6000) {
            foot[1] = below[1];
            contact = (support & ~0x8000u) | (contact & 0x8000);
        } else {
            contact = 0;
            material = 0;
        }
    }
    if (mode == 0 && !contact) material = 0;
    foot[1] = (int16_t)(foot[1]-vertical_offset);
    for (unsigned i = 0; i < 3; ++i) {
        m->position[i] = foot[i] * 65536 + (uint16_t)m->position[i];
        m->xyz[i] = foot[i];
    }
    if (contact & 0x6000) m->velocity[1] = 0;
    return contact | (material & 255);
}

static unsigned mine387_travel(Mine387Motion *m, const BfmVillageGround *ground, unsigned mode)
{
    return mine387_travel_at(m,ground,mode,0);
}

static void mine387_rollback(Mine387Motion *m, const int32_t previous[3])
{
    memcpy(m->position,previous,sizeof m->position);
    for (unsigned i = 0; i < 3; ++i) m->xyz[i] = (int16_t)mine387_down(previous[i],65536);
}

static void mine387_patrol(Mine387Motion *m, const BfmVillageGround *ground,
                           const int16_t aim[3], BfmGameplayRun *run,
                           const BfmForestTransform *submitted)
{
    int wave = mine387_wave((m->phase * 64) & 1984);
    m->scale_y = (int16_t)(2048 + wave / 2);
    if (m->patrol == M387_TURN_BACK) {
        int step = mine387_steer(m->turn_yaw,m->yaw,10);
        m->yaw = (int16_t)(m->yaw+step);
        if (!step) m->patrol = M387_STRAIGHT;
    } else {
        int32_t previous[3];
        memcpy(previous,m->position,sizeof previous);
        int straight = m->patrol == M387_STRAIGHT;
        if (!straight) m->yaw = (int16_t)(m->yaw + mine387_steer(m->turn_yaw,m->yaw,20));
        /* The ordinary field commits new Euler angles after this dispatch.
         * The stride still uses the matrix from the previous field. */
        mine387_stride(m,submitted,-(straight ? 32768 : 16384) - 16*(4096-wave));
        unsigned contact = mine387_travel(m,ground,0);
        if ((contact & 255) == 26) {
            m->action = M387_GONE;
            return;
        }
        if (!(contact & 0x6000)) {
            mine387_rollback(m,previous);
            m->turn_yaw = (int16_t)(m->yaw + 2048);
            m->patrol = M387_TURN_BACK;
        } else if (straight) {
            if (--m->straight_left == 0) {
                int turn = bfm_gameplay_random(run) & 1023;
                if (!(bfm_gameplay_random(run) & 1)) turn = -turn;
                m->turn_yaw = (int16_t)(m->yaw + turn);
                m->timer = 32;
                m->patrol = M387_WANDER;
            }
        } else if (m->timer && --m->timer == 0) {
            m->straight_left = 64;
            m->patrol = M387_STRAIGHT;
        }
    }
    if (mine387_sees(m,aim)) m->action = M387_CHASE;
    ++m->phase;
}

static void mine387_chase(Mine387Motion *m, const BfmVillageGround *ground, const int16_t aim[3],
                          const BfmForestTransform *submitted)
{
    if (!mine387_sees(m,aim)) {
        m->action = M387_START;
        return;
    }
    if (mine387_distance2(m,aim,0) <= 128*128) {
        m->action = M387_AIM;
        return;
    }
    int wave = mine387_wave((m->phase * 128) & 1920);
    m->scale_y = (int16_t)(2048 + wave/2);
    int32_t previous[3];
    memcpy(previous,m->position,sizeof previous);
    m->yaw = (int16_t)(m->yaw + mine387_steer(mine387_heading(m,aim),m->yaw,10));
    mine387_stride(m,submitted,-65536 - 128*(4096-wave));
    if (!(mine387_travel(m,ground,0) & 0x6000)) mine387_rollback(m,previous);
    ++m->phase;
}

static void mine387_aim(Mine387Motion *m, const int16_t hero[3])
{
    m->yaw = (int16_t)mine387_heading(m,hero);
    int dx = hero[0]-m->xyz[0], dz = hero[2]-m->xyz[2];
    int along = -mine387_down(dx*mine387_wave((unsigned)m->yaw) +
                              dz*mine387_wave((unsigned)m->yaw+1024),4096);
    m->pitch = (int16_t)bfm_waypoint_heading(hero[1]-48-m->xyz[1],along);
    /* This helper returns a signed pitch for a target above the actor. */
    if (m->pitch > 2048) m->pitch -= 4096;
    m->stop_y = (int16_t)(hero[1]-96);
    m->gravity = 0;
    mine387_forward(m,m->pitch,m->yaw,-16*65536);
    m->timer = 0;
    m->flight = M387_OUTWARD;
    m->action = M387_LUNGE;
}

static void mine387_begin_common_fall(Mine387Motion *m)
{
    m->common_fall = 1;
    m->timer = 60;
}

static unsigned mine387_lunge(Mine387Motion *m, const BfmVillageGround *ground, const int16_t hero[3])
{
    unsigned mode = 0, hit = 0;
    switch (m->flight) {
    case M387_OUTWARD: case M387_FALL: mode = 17; break;
    case M387_OFF_EDGE: mode = 1; break;
    case M387_SKID:
        m->velocity[0] = m->velocity[0] * 15 / 16;
        m->velocity[2] = m->velocity[2] * 15 / 16;
        break;
    case M387_WALL_FALL: m->dust = m->timer % 3 == 0; break;
    }
    unsigned contact = mine387_travel(m,ground,mode);
    if ((contact & 255) == 26) {
        m->action = M387_GONE;
        return 0;
    }
    switch (m->flight) {
    case M387_OUTWARD:
        if (contact & 0x4000) break;
        if (mine387_distance2(m,hero,-64) < 64*64) {
            m->hit_yaw = m->yaw;
            hit = 1;
            m->pitch = (int16_t)(m->pitch + 2048);
            mine387_forward(m,m->pitch,m->yaw,-16*65536);
            m->gravity = 98304;
            m->flight = M387_FALL;
        } else {
            if (m->velocity[1]<0 ? m->xyz[1]<m->stop_y : m->xyz[1]>m->stop_y) {
                m->gravity = 98304;
                m->flight = M387_FALL;
            }
            ++m->timer;
        }
        return hit;
    case M387_FALL:
        if (contact & 0x2000) {
            m->pitch = 0;
            m->timer = 0;
            m->flight = M387_SKID;
            return 0;
        }
        if (contact & 0x4000) break;
        if (++m->timer >= 60) mine387_begin_common_fall(m);
        return 0;
    case M387_SKID:
        if (!(contact & 0x6000)) {
            m->timer = 0;
            m->flight = M387_OFF_EDGE;
        } else {
            m->dust = m->timer % 3 == 0;
            if (++m->timer >= 16) m->action = M387_START;
        }
        return 0;
    case M387_OFF_EDGE:
        if (contact & 0x4000) break;
        if (contact & 0x2000) {
            m->action = M387_START;
            m->dust = 1;
        } else if (++m->timer >= 60) mine387_begin_common_fall(m);
        return 0;
    case M387_WALL_FALL:
        if (contact & 0x2000) m->action = M387_START;
        else if (++m->timer >= 60) mine387_begin_common_fall(m);
        return 0;
    }
    m->velocity[0] = m->velocity[1] = m->velocity[2] = 0;
    m->gravity = 98304;
    m->pitch = 0;
    m->flight = M387_WALL_FALL;
    return 0;
}

/* Hero is the actual published foot. Aim is the separate target service
 * position, and is NULL when that service suppresses an acquisition. */
static unsigned mine387_motion_step(Mine387Motion *m, const BfmVillageGround *ground,
    const int16_t hero[3], const int16_t aim[3], BfmGameplayRun *run,
    const BfmForestTransform *submitted)
{
    unsigned hit = 0;
    m->dust = 0;
    if (m->action == M387_GONE) return 0;
    if ((m->action == M387_PATROL || m->action == M387_CHASE || m->action == M387_LUNGE) &&
        m->xyz[1] >= 16) {
        m->action = M387_GONE;
        return 0;
    }
    switch (m->action) {
    case M387_START:
        m->gravity = 98304;
        mine387_stride(m,submitted,-163840);
        m->phase = 32;
        m->patrol = M387_STRAIGHT;
        m->action = M387_PATROL;
        break;
    case M387_PATROL: mine387_patrol(m,ground,aim,run,submitted); break;
    case M387_CHASE: mine387_chase(m,ground,aim,submitted); break;
    case M387_AIM: mine387_aim(m,hero); break;
    case M387_LUNGE: hit = mine387_lunge(m,ground,hero); break;
    case M387_GONE: break;
    }
    if (m->velocity[1] > 16*65536) m->velocity[1] = 16*65536;
    return hit;
}

typedef enum { M387_CEILING_HIDDEN, M387_CEILING_DROP, M387_CEILING_RELEASED } Mine387CeilingPhase;

typedef struct {
    Mine387CeilingPhase phase;
    int16_t scale_xz;
} Mine387Ceiling;

typedef enum {
    M387_CEILING_WAIT, M387_CEILING_SHOWN, M387_CEILING_FALLING, M387_CEILING_LANDED
} Mine387CeilingEdge;

/* The side-room placement allocator owns when to call this constructor.
 * The first visible field uses the ordinary uncompressed model. */
static void mine387_ceiling_start(Mine387Ceiling *drop)
{
    *drop = (Mine387Ceiling){.phase=M387_CEILING_HIDDEN, .scale_xz=4096};
}

/* Update after class initialization. Once RELEASED, use the ordinary motion
 * dispatch starting on the next field, preserving the last X/Z scale. */
static Mine387CeilingEdge mine387_ceiling_step(Mine387Ceiling *drop, Mine387Motion *m,
    const BfmVillageGround *ground, const int16_t hero[3])
{
    if (drop->phase == M387_CEILING_HIDDEN) {
        int dx = (int)hero[0]-m->xyz[0], dz = (int)hero[2]-m->xyz[2];
        if ((int64_t)dx*dx + (int64_t)dz*dz > 512*512) return M387_CEILING_WAIT;
        m->velocity[0] = m->velocity[1] = m->velocity[2] = 0;
        m->gravity = 49152;
        drop->phase = M387_CEILING_DROP;
        return M387_CEILING_SHOWN;
    }
    int pulse = mine387_wave((m->phase*64u)&1984u);
    drop->scale_xz = (int16_t)(3584 + pulse/8);
    m->scale_y = (int16_t)(2048 + pulse/2);
    unsigned contact = mine387_travel(m,ground,17);
    ++m->phase;
    if (!(contact & 0x2000)) return M387_CEILING_FALLING;
    drop->phase = M387_CEILING_RELEASED;
    m->action = M387_START;
    m->dust = 1;
    return M387_CEILING_LANDED;
}

/* Landing pan for the published projection relative to the display center.
 * Call only on LANDED; the dust and transition are unconditional. */
static int mine387_ceiling_land_pan(int x, int y)
{
    if (x < -180 || x > 180 || y < -140 || y > 140) return -1;
    int pan = 64 + x*64/180;
    return pan > 127 ? 127 : pan;
}

enum MineRecoil { NO_RECOIL, TUMBLE, SLIDE, OVER_EDGE, WAIT, WALL };

typedef struct {
    Mine387Motion motion;
    Mine387Ceiling ceiling;
    enum MineRecoil recoil;
    unsigned last_cut, cut_yaw, cut_strength;
    int pending_cut, cut_pose_active, hidden;
    int16_t home[3], halo_xyz[3];
    BfmForestTransform submitted;
    unsigned spawn_yaw, generation, halo_tick;
    int halo_visible;
    int allocated;
} MineResident;

struct BfmVillageMineCorridor387 {
    BfmForestGuardBox body;
    int32_t recoil_vector[3];
    int16_t recoil_foot_y;
    uint16_t cut_sounds[3];
    MineResident fixed, ceiling[BFM_MINE_CEILING_ROWS];
    const BfmVillageGround *ground;
    const uint8_t *scene;
    size_t scene_bytes;
    uint32_t room_address;
    unsigned row_count;
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose, cut_pose;
    BfmBedroomLight field_lamps[3], lamps[3];
    uint8_t shadow_texture[12], center[3], edge[3];
    int16_t attachment[3], rim[12][2];
    int initialized;
};

/* The halo child reads the matrix published by the last field. A lunge's
 * initial aiming turn and its contact rebound publish an unscaled matrix
 * inside the update. The renderer publishes the new scaled body afterward. */
enum PublishedPose { PREVIOUS_POSE, TURN_NOW, PREPARED_POSE };

static void publish_shape(MineResident *c, const int16_t attachment[3], enum PublishedPose pose,
                          unsigned tick, int update_halo)
{
    const Mine387Motion *m = &c->motion;
    int visible = c->allocated && !c->hidden;
    if (pose == TURN_NOW) mine387_aim_basis(m,m->pitch,m->yaw,&c->submitted);
    if (update_halo) {
        bfm_forest_lift_world(&c->submitted,attachment,c->halo_xyz);
        c->halo_visible = visible;
        c->halo_tick = tick;
    }
    if (visible) {
        if (pose == PREVIOUS_POSE)
            bfm_forest_actor_pitch_yaw(m->pitch,m->yaw,m->xyz,&c->submitted);
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned k = 0; k < 3; ++k) {
                int size = k == 1 ? m->scale_y : c->ceiling.scale_xz;
                c->submitted.rotation[i][k] = (int16_t)mine387_down(c->submitted.rotation[i][k]*size,4096);
            }
        }
    }
}

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || count > bytes || address - base > bytes - count) return NULL;
    return scene + (address - base);
}

typedef struct {
    uint32_t binding, room, profile, shadow;
    unsigned fixed_variant, ceiling_rows;
} MineCreatureLayout;

static BfmVillageMineCorridor387 *open_creatures(const uint8_t *scene, size_t bytes,
    const BfmVillageGround *ground, const MineCreatureLayout *layout)
{
    const uint8_t *binding = mounted(scene, bytes, layout->binding, 4);
    if (!ground || !binding || u32le(binding) != layout->room) return NULL;
    const uint8_t *room = mounted(scene, bytes, layout->room, 0xbc);
    uint32_t config = layout->profile;
    const uint8_t *profile = mounted(scene, bytes, config, 0x34);
    const uint8_t *lights = mounted(scene, bytes, config + 0x34, 48);
    const uint8_t *halo = mounted(scene, bytes, config + 0x7c, 64);
    const uint8_t *anchor = mounted(scene, bytes, config + 0xbc, 6);
    const uint8_t *shadow = mounted(scene, bytes, layout->shadow, 12);
    if (!room || !profile || !lights || !halo || !anchor || !shadow) return NULL;

    const uint8_t *clip = mounted(scene,bytes,config + 0x6c,8);
    const uint8_t *cut_pose = clip && (u32le(clip) & UINT32_C(0x01000000)) ?
        mounted(scene,bytes,u32le(clip) & ~UINT32_C(0x01000000),8) : NULL;
    const uint8_t *bounds = mounted(scene,bytes,u32le(profile+8),16);
    const uint8_t *impulse = mounted(scene,bytes,config + 0x194,12);
    const uint8_t *foot = mounted(scene,bytes,config + 0xc4,6);
    if (!bounds || u32le(bounds) || !impulse || !foot || s16le(foot) || s16le(foot+4) ||
        (u16le(profile+2) & 0x8100) != 0x8000) return NULL;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (first >= end || (end-first)%20) return NULL;
    const uint8_t *rows = mounted(scene, bytes, first, end-first);
    const uint8_t *placed = NULL;
    if (!rows) return NULL;
    for (size_t i = 0; i < end-first && u16le(rows+i+6); i += 20) {
        if (u16le(rows+i+6) == 387) {
            if (placed || u16le(rows+i+8) != layout->fixed_variant) return NULL;
            placed = rows+i;
        }
    }
    if (!placed) return NULL;

    BfmVillageMineCorridor387 *a = calloc(1, sizeof *a);
    if (!a) return NULL;
    a->recoil_foot_y = s16le(foot+2);
    for (unsigned i = 0; i < 3; ++i) {
        a->body.low[i] = s16le(bounds+4+4*i);
        a->body.high[i] = s16le(bounds+6+4*i);
        if (a->body.low[i] > a->body.high[i]) goto fail;
        a->recoil_vector[i] = (int32_t)u32le(impulse+4*i);
        a->cut_sounds[i] = u16le(profile+20+2*i);
    }
    if (!bfm_forest_actor_pose_decode(cut_pose,8,1,&a->cut_pose)) goto fail;
    unsigned parts = 0;
    a->mesh = musashi_forest_player_mesh_from_table(scene, bytes, UINT32_C(0x80128158),
                                                   u32le(profile+4), &parts);
    if (!a->mesh || parts != 1) goto fail;
    bfm_forest_actor_root(0, (const int16_t[3]){0}, &a->pose.local[0]);
    a->ground = ground;
    a->scene = scene;
    a->scene_bytes = bytes;
    a->room_address = layout->room;
    a->row_count = layout->ceiling_rows;
    for (unsigned i = 0; i < 3; ++i) {
        a->fixed.home[i] = s16le(placed + 2*i);
        a->attachment[i] = s16le(anchor + 2*i);
        a->center[i] = halo[i];
        a->edge[i] = halo[4+i];
        const uint8_t *field = room + 6 + 10*i;
        for (unsigned k = 0; k < 3; ++k) {
            int32_t axis = (int32_t)u32le(lights + 16*i + 4*k);
            if (axis < INT16_MIN || axis > INT16_MAX) goto fail;
            a->lamps[i].axis[k] = (int16_t)axis;
            a->field_lamps[i].axis[k] = s16le(field + 2*k);
        }
        memcpy(a->field_lamps[i].rgb, field+6, 3);
        memcpy(a->lamps[i].rgb, lights+16*i+12, 3);
    }
    for (unsigned i = 0; i < 12; ++i)
        for (unsigned k = 0; k < 2; ++k)
            a->rim[i][k] = s16le(halo + 16 + 4*i + 2*k);
    memcpy(a->shadow_texture, shadow, sizeof a->shadow_texture);
    a->fixed.spawn_yaw = u16le(placed+12);
    mine387_motion_start(&a->fixed.motion, a->fixed.home, a->fixed.spawn_yaw);
    if (layout->fixed_variant & 2) {
        a->fixed.hidden = 1;
        mine387_ceiling_start(&a->fixed.ceiling);
    } else {
        a->fixed.ceiling = (Mine387Ceiling){.phase=M387_CEILING_RELEASED, .scale_xz=4096};
    }
    bfm_forest_actor_root(0,(const int16_t[3]){0},&a->fixed.submitted);
    const uint8_t *ring = mounted(scene,bytes,end,20*(a->row_count+1));
    if (!ring || u16le(ring+20*a->row_count+6)) goto fail;
    for (unsigned i = 0; i < a->row_count; ++i) {
        const uint8_t *row = ring+20*i;
        MineResident *c = a->ceiling+i;
        if (u16le(row+6) != 387 || u16le(row+8) != 0x12) goto fail;
        for (unsigned k = 0; k < 3; ++k) c->home[k] = s16le(row+2*k);
        c->spawn_yaw = u16le(row+12);
    }
    return a;
fail:
    bfm_village_mine_corridor_387_free(a);
    return NULL;
}

BfmVillageMineCorridor387 *bfm_village_mine_corridor_387_open(const uint8_t *scene,
    size_t bytes, const BfmVillageGround *ground)
{
    const MineCreatureLayout layout = {
        0x80180c54u, 0x80184870u, 0x8019e640u, 0x801825b8u, 0, 3
    };
    return open_creatures(scene, bytes, ground, &layout);
}

BfmVillageMineCorridor387 *bfm_village_mine_lake_387_open(const uint8_t *scene,
    size_t bytes, const BfmVillageGround *ground)
{
    const MineCreatureLayout layout = {
        0x80181d20u, 0x80185a50u, 0x8019bf40u, 0x80183684u, 2, 2
    };
    return open_creatures(scene, bytes, ground, &layout);
}

void bfm_village_mine_corridor_387_free(BfmVillageMineCorridor387 *a)
{
    if (a) musashi_forest_player_mesh_free(a->mesh);
    free(a);
}

static void update_light(BfmVillageMineCorridor387 *a, unsigned world_tick)
{
    uint8_t lamp[3];
    bfm_village_mine_corridor_glow(world_tick,lamp,a->center);
    for (unsigned i = 0; i < 3; ++i) memcpy(a->lamps[i].rgb,lamp,3);
}

void bfm_village_mine_corridor_387_enter(BfmVillageMineCorridor387 *a,
    const int16_t hero[3], unsigned world_tick)
{
    if (!a || !hero || a->initialized) return;
    a->initialized = 1;
    ++a->fixed.generation;
    update_light(a,world_tick);
    a->fixed.allocated = mine387_distance2(&a->fixed.motion,hero,0) < 2048*2048;
    publish_shape(&a->fixed,a->attachment,PREVIOUS_POSE,world_tick,1);
}

static void mine387_touchdown(Mine387Motion *m, const BfmVillageMineCorridor387 *room)
{
    m->common_fall = 0;
    m->action = M387_START;
    m->pitch = 0;
    /* The shared projection starts 20 units above its supplied root. The
     * recovery snaps vertically from four above the foot through 764 below. */
    BfmBedroomRoot foot = {.xyz = {m->xyz[0], (int16_t)(m->xyz[1] + 16), m->xyz[2]}};
    if (bfm_bedroom_travel_air_shadow(room->scene,room->scene_bytes,room->room_address,&foot) &&
        foot.shadow_visible) {
        for (unsigned i = 0; i < 3; ++i) {
            m->xyz[i] = foot.shadow_xyz[i];
            m->position[i] = (int32_t)foot.shadow_xyz[i] * 65536;
        }
    }
}

static void mine387_common_fall_step(Mine387Motion *m, const BfmVillageMineCorridor387 *room)
{
    m->dust = 0;
    if (m->xyz[1] < 16 && m->timer && --m->timer) {
        unsigned contact = mine387_travel(m,room->ground,17);
        if ((contact & 255) != 28) {
            if (contact & 0x2000) mine387_touchdown(m,room);
            return;
        }
    }
    m->common_fall = 0;
    m->action = M387_GONE;
}

static int blade_resident(const BfmVillageMineCorridor387 *a, MineResident *c,
    unsigned event, unsigned strength, unsigned hero_yaw, const int16_t from[3],
    int16_t to[3], int16_t normal[3])
{
    if (!c->allocated || !from || !to || c->last_cut == event ||
        !((event >= 1 && event <= 4) || event == 18 || event == 19)) return 0;

    BfmForestGuardBox box = a->body;
    BfmForestTransform basis = c->submitted;
    for (unsigned col = 0; col < 3; ++col) {
        int scale = col == 1 ? c->motion.scale_y : c->ceiling.scale_xz;
        int reciprocal = (1 << 24)/scale;
        box.low[col] = (int16_t)mine387_down(box.low[col]*scale,4096);
        box.high[col] = (int16_t)mine387_down(box.high[col]*scale,4096);
        for (unsigned row = 0; row < 3; ++row)
            basis.rotation[row][col] = (int16_t)mine387_down(
                basis.rotation[row][col]*reciprocal,4096);
    }
    int16_t center[3];
    if (!bfm_forest_guard_contact_point(&box,&basis,from,to,center,to,normal)) return 0;
    c->last_cut = event;
    c->cut_yaw = hero_yaw;
    c->cut_strength = strength;
    c->pending_cut = 1;
    return 1;
}

int bfm_village_mine_corridor_387_blade(BfmVillageMineCorridor387 *a,
    unsigned event, unsigned strength, unsigned hero_yaw, const int16_t from[3],
    int16_t to[3], int16_t normal[3])
{
    return a && blade_resident(a,&a->fixed,event,strength,hero_yaw,from,to,normal);
}

int bfm_village_mine_corridor_387_blade_row(BfmVillageMineCorridor387 *a, unsigned row,
    unsigned event, unsigned strength, unsigned hero_yaw, const int16_t from[3],
    int16_t to[3], int16_t normal[3])
{
    return a && row < a->row_count &&
        blade_resident(a,&a->ceiling[row],event,strength,hero_yaw,from,to,normal);
}

static int resident_dust(const MineResident *c, int16_t xyz[3])
{
    if (!c->allocated || !xyz || !c->motion.dust) return 0;
    memcpy(xyz,c->motion.xyz,sizeof c->motion.xyz);
    return 1;
}

int bfm_village_mine_corridor_387_dust(const BfmVillageMineCorridor387 *a, int16_t xyz[3])
{
    return a && resident_dust(&a->fixed,xyz);
}

int bfm_village_mine_corridor_387_dust_row(const BfmVillageMineCorridor387 *a, unsigned row,
    int16_t xyz[3])
{
    return a && row < a->row_count && resident_dust(&a->ceiling[row],xyz);
}

unsigned bfm_village_mine_corridor_387_blade_sound(const BfmVillageMineCorridor387 *a,
    unsigned event)
{
    if (!a) return 0;
    switch (event) {
    case 1: case 2: case 18: case 19: return a->cut_sounds[0];
    case 3: return a->cut_sounds[1];
    case 4: return a->cut_sounds[2];
    default: return 0;
    }
}

/* This creature reacts to an ordinary blade by tumbling away. Its own
 * receiver handles the impact and keeps its health throughout the tumble. */
static void begin_recoil(const BfmVillageMineCorridor387 *a, MineResident *c)
{
    Mine387Motion *m = &c->motion;
    m->yaw = (int16_t)((c->cut_yaw+2048)&4095u);
    m->pitch = 0;
    m->timer = 0;
    m->gravity = 98304;
    mine387_aim_basis(m,m->pitch,m->yaw,&c->submitted);
    c->pending_cut = 0;
    m->common_fall = 0;
    if (c->last_cut == 1 && !c->cut_strength) {
        memset(m->velocity,0,sizeof m->velocity);
        c->recoil = WAIT;
        return;
    }
    for (unsigned row = 0; row < 3; ++row) {
        int64_t value = 0;
        for (unsigned col = 0; col < 3; ++col)
            value += (int64_t)c->submitted.rotation[row][col]*a->recoil_vector[col];
        m->velocity[row] = mine387_down(value,4096);
    }
    c->recoil = TUMBLE;
    c->cut_pose_active = 1;
}

static void recover_from_cut(const BfmVillageMineCorridor387 *a, MineResident *c)
{
    c->last_cut = 0;
    c->recoil = NO_RECOIL;
    c->cut_pose_active = 0;
    c->ceiling.phase = M387_CEILING_RELEASED;
    mine387_touchdown(&c->motion,a);
}

static void recoil_step(const BfmVillageMineCorridor387 *a, MineResident *c)
{
    Mine387Motion *m = &c->motion;
    m->dust = 0;
    if (m->xyz[1] >= 16) {
        m->action = M387_GONE;
        return;
    }
    if (c->recoil == WAIT) {
        if (++m->timer >= 16) recover_from_cut(a,c);
        return;
    }
    if (c->recoil == TUMBLE) m->pitch = (int16_t)(m->pitch-256);
    if (c->recoil == SLIDE) {
        m->velocity[0] = 15*m->velocity[0]/16;
        m->velocity[2] = 15*m->velocity[2]/16;
    }
    unsigned mode = c->recoil == SLIDE || c->recoil == WALL ? 0 : 17;
    unsigned contact = mine387_travel_at(m,a->ground,mode,a->recoil_foot_y);
    if ((contact & 255) == 26 && c->recoil != OVER_EDGE) {
        m->action = M387_GONE;
        return;
    }
    if (c->recoil == TUMBLE && (contact & 0x2000)) {
        c->recoil = SLIDE;
        m->pitch = 0;
        m->timer = 0;
    } else if ((c->recoil == TUMBLE || c->recoil == OVER_EDGE) && (contact & 0x4000)) {
        m->dust = c->recoil == OVER_EDGE;
        if (c->recoil == TUMBLE) m->timer = 0;
        c->recoil = WALL;
        m->pitch = 0;
        memset(m->velocity,0,sizeof m->velocity);
    } else if (c->recoil == SLIDE) {
        if (!(contact & 0x6000)) {
            c->recoil = OVER_EDGE;
            m->timer = 0;
            memset(m->velocity,0,sizeof m->velocity);
        } else {
            m->dust = m->timer % 4 == 0;
            if (++m->timer >= 16) recover_from_cut(a,c);
        }
    } else if (c->recoil != TUMBLE && (contact & 0x2000)) {
        m->dust = c->recoil == OVER_EDGE;
        recover_from_cut(a,c);
    } else {
        if (c->recoil == WALL) m->dust = m->timer % 4 == 0;
        if (++m->timer >= 60) {
            if (c->recoil == TUMBLE) c->recoil = OVER_EDGE;
            else {
                mine387_begin_common_fall(m);
                c->recoil = NO_RECOIL;
            }
        }
    }
    if (m->velocity[1] > 16*65536) m->velocity[1] = 16*65536;
}

static unsigned active_step(MineResident *resident, const BfmVillageMineCorridor387 *room,
    const int16_t hero[3], const int16_t target[3], BfmGameplayRun *run, BfmMineLungeHit *hit)
{
    Mine387Motion *m = &resident->motion;
    if (m->common_fall) {
        mine387_common_fall_step(m,room);
        return 0;
    }
    unsigned struck = mine387_motion_step(m,room->ground,hero,target,run,&resident->submitted);
    if (struck && hit) {
        memcpy(hit->origin,m->xyz,sizeof hit->origin);
        hit->yaw = (unsigned)m->hit_yaw & 4095u;
        hit->flags = 17;
        hit->power = 5;
    }
    return struck;
}

/* Returning to a ceiling placement keeps the fractional position and scales.
 * The hidden model still publishes an unscaled matrix at its home. */
static void return_to_ceiling(MineResident *c)
{
    Mine387Motion *m = &c->motion;
    for (unsigned k = 0; k < 3; ++k) {
        m->position[k] = c->home[k]*65536 + (uint16_t)m->position[k];
        m->xyz[k] = c->home[k];
    }
    m->pitch = m->yaw = 0;
    m->action = M387_START;
    m->common_fall = 0;
    c->recoil = NO_RECOIL;
    c->last_cut = 0;
    c->hidden = 1;
    c->ceiling.phase = M387_CEILING_HIDDEN;
}

static int step_resident(BfmVillageMineCorridor387 *a, MineResident *c,
    const int16_t hero[3], const int16_t published[3], const int16_t target[3], BfmGameplayRun *run, unsigned tick,
    BfmMineLungeHit *hit, BfmMineCreatureLanding *landing)
{
    if (!c->allocated) return 0;
    update_light(a,tick);
    Mine387Motion *m = &c->motion;
    enum PublishedPose pose = PREVIOUS_POSE;
    int class_step = !c->pending_cut && c->recoil == NO_RECOIL && !m->common_fall;
    int struck = 0, update_halo = 1;
    if (c->pending_cut || c->recoil != NO_RECOIL) {
        if (c->pending_cut) {
            begin_recoil(a,c);
            pose = PREPARED_POSE;
        }
        recoil_step(a,c);
    } else if (m->common_fall || c->ceiling.phase == M387_CEILING_RELEASED) {
        if (class_step && m->action == M387_AIM) pose = TURN_NOW;
        struck = active_step(c,a,hero,target,run,hit);
        if (struck) pose = TURN_NOW;
        if (!class_step && !m->common_fall && m->action != M387_GONE) {
            c->last_cut = 0;
            c->cut_pose_active = 0;
            c->ceiling.phase = M387_CEILING_RELEASED;
        }
    } else {
        m->dust = 0;
        Mine387CeilingEdge edge = mine387_ceiling_step(&c->ceiling,m,a->ground,hero);
        if (edge == M387_CEILING_SHOWN) {
            c->hidden = 0;
            if (landing) landing->request_shadow = 1;
        }
        if (edge == M387_CEILING_LANDED && landing) {
            landing->landed = 1;
            memcpy(landing->xyz,m->xyz,sizeof landing->xyz);
        }
    }
    if (m->action == M387_GONE) {
        if (c == &a->fixed) c->allocated = 0;
        else {
            if (landing) landing->release_shadow = !c->hidden;
            /* A common fall or tumble exits directly on rehang. The class
             * routine updates its separate halo on the next occupied pass. */
            update_halo = class_step;
            return_to_ceiling(c);
            pose = TURN_NOW;
        }
    }
    if (class_step && mine387_distance2(m,published,0) >= 2048*2048) c->allocated = 0;
    publish_shape(c,a->attachment,pose,tick,update_halo);
    return struck;
}

int bfm_village_mine_corridor_387_step(BfmVillageMineCorridor387 *a,
    const int16_t hero[3], const int16_t target[3], BfmGameplayRun *run,
    unsigned world_tick, BfmMineLungeHit *hit)
{
    if (!a || !hero || !run) return 0;
    if (!a->initialized) {
        bfm_village_mine_corridor_387_enter(a,hero,world_tick);
        return 0;
    }
    return step_resident(a,&a->fixed,hero,hero,target,run,world_tick,hit,NULL);
}

int bfm_village_mine_corridor_387_wants_row(const BfmVillageMineCorridor387 *a,
    unsigned row, const int16_t reference[3])
{
    if (!a || !reference || row >= a->row_count || a->ceiling[row].allocated) return 0;
    const int16_t *home = a->ceiling[row].home;
    int64_t dx = (int)reference[0]-home[0], dy = (int)reference[1]-home[1];
    int64_t dz = (int)reference[2]-home[2], distance = dx*dx+dy*dy+dz*dz;
    return distance > 1536*1536 && distance < 1600*1600;
}

int bfm_village_mine_corridor_387_spawn_row(BfmVillageMineCorridor387 *a,
    unsigned row, const int16_t reference[3])
{
    if (!bfm_village_mine_corridor_387_wants_row(a,row,reference)) return 0;
    MineResident *c = a->ceiling+row;
    c->last_cut = 0;
    c->pending_cut = c->cut_pose_active = 0;
    c->recoil = NO_RECOIL;
    c->hidden = 1;
    c->halo_visible = 0;
    mine387_motion_start(&c->motion,c->home,c->spawn_yaw);
    mine387_ceiling_start(&c->ceiling);
    ++c->generation;
    bfm_forest_actor_root(0,(const int16_t[3]){0},&c->submitted);
    c->allocated = 1;
    return 1;
}

int bfm_village_mine_corridor_387_has_row(const BfmVillageMineCorridor387 *a, unsigned row)
{
    return a && row < a->row_count && a->ceiling[row].allocated;
}

int bfm_village_mine_corridor_387_visible_row(const BfmVillageMineCorridor387 *a, unsigned row)
{
    return bfm_village_mine_corridor_387_has_row(a,row) && !a->ceiling[row].hidden;
}

int bfm_village_mine_corridor_387_step_row(BfmVillageMineCorridor387 *a, unsigned row,
    const int16_t hero[3], const int16_t target[3], BfmGameplayRun *run, unsigned tick,
    BfmMineLungeHit *hit, BfmMineCreatureLanding *landing)
{
    if (landing) *landing = (BfmMineCreatureLanding){0};
    if (!a || row >= a->row_count || !hero || !run) return 0;
    return step_resident(a,&a->ceiling[row],hero,hero,target,run,tick,hit,landing);
}

int bfm_village_mine_corridor_387_step_actor(BfmVillageMineCorridor387 *a, unsigned parent,
    const int16_t hero[3], const int16_t published[3], const int16_t target[3],
    BfmGameplayRun *run, unsigned tick, BfmMineLungeHit *hit, BfmMineCreatureLanding *landing)
{
    if (landing) *landing = (BfmMineCreatureLanding){0};
    if (!a || (parent != BFM_MINE_WEST_CREATURE && parent >= a->row_count) || !hero || !published || !run) return 0;
    MineResident *c = parent == BFM_MINE_WEST_CREATURE ? &a->fixed : &a->ceiling[parent];
    return step_resident(a,c,hero,published,target,run,tick,hit,landing);
}

int bfm_village_mine_corridor_387_landing_pan(const BfmMineCreatureLanding *landing,
    const BfmBedroomView *view)
{
    if (!landing || !landing->landed || !view) return -1;
    int center[3];
    bfm_bedroom_project(view,landing->xyz,center);
    return mine387_ceiling_land_pan(center[0]-160,center[1]-120);
}

static uint32_t rgb24(const uint8_t c[3])
{
    return (uint32_t)c[0] | (uint32_t)c[1]<<8 | (uint32_t)c[2]<<16;
}

/* The halo is a camera-facing fan. Project the attachment once, then project
 * the scaled rim in eye space at that same depth and add its screen offset. */
static int draw_halo(const BfmVillageMineCorridor387 *a, const MineResident *c,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    int hub[3];
    if (!c->halo_visible || !bfm_bedroom_project_packet(view,c->halo_xyz,hub) ||
        hub[2]/4+1 >= 4096) return 1;
    unsigned angle = (c->halo_tick*56u)&2040u;
    int wave = mine387_wave(angle);
    int scale = 1109 + mine387_down(wave, 16);
    int divider = (view->focal*65536 + hub[2]/2)/hub[2];
    if (divider > 131071) divider = 131071;
    BfmForestRasterVertex rim[12];
    for (unsigned i = 0; i < 12; ++i) {
        int x = mine387_down(scale*a->rim[i][0],4096);
        int y = mine387_down(scale*a->rim[i][1],4096);
        rim[i] = (BfmForestRasterVertex){.x=hub[0]+mine387_down((int64_t)x*divider,65536),
            .y=hub[1]+mine387_down((int64_t)y*divider,65536), .rgb=rgb24(a->edge)};
    }
    BfmForestRasterVertex center = {.x=hub[0],.y=hub[1],.rgb=rgb24(a->center)};
    BfmForestDustCard cards[6];
    for (unsigned i = 0; i < 6; ++i) {
        cards[i] = (BfmForestDustCard){
            .vertices={rim[2*i],rim[2*i+1],center,rim[(2*i+2)%12]},
            .bucket=(unsigned)(hub[2]/4+1), .material={.count=4,.translucent=1,.page=32}};
    }
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, 6);
}

static const MineResident *shadow_parent(const BfmVillageMineCorridor387 *a, unsigned parent)
{
    if (!a || (parent != BFM_MINE_WEST_CREATURE && parent >= a->row_count)) return NULL;
    return parent == BFM_MINE_WEST_CREATURE ? &a->fixed : &a->ceiling[parent];
}

int bfm_village_mine_corridor_387_shadow_start(const BfmVillageMineCorridor387 *a, unsigned parent,
    BfmMineCreatureShadow *shadow)
{
    const MineResident *c = shadow_parent(a,parent);
    if (!shadow || !c || !c->generation || c->hidden) return 0;
    /* The west constructor can create a shadow and cull its own body in one
     * call. The child still occupies the granted slot until its first update. */
    *shadow = (BfmMineCreatureShadow){.parent=parent,.generation=c->generation,.active=1};
    memcpy(shadow->floor.xyz,c->motion.xyz,sizeof shadow->floor.xyz);
    memcpy(shadow->floor.shadow_xyz,c->motion.xyz,sizeof shadow->floor.shadow_xyz);
    shadow->floor.shadow_visible = 1;
    shadow->floor.shadow_normal[1] = -4096;
    shadow->at_birth = 1;
    return 1;
}

int bfm_village_mine_corridor_387_shadow_step(const BfmVillageMineCorridor387 *a,
    BfmMineCreatureShadow *shadow)
{
    if (!shadow || !shadow->active) return 0;
    const MineResident *c = shadow_parent(a,shadow->parent);
    if (!c || !c->allocated || c->generation != shadow->generation || c->hidden) {
        shadow->active = 0;
        return 0;
    }
    shadow->at_birth = 0;
    shadow->floor.yaw = (uint16_t)c->motion.yaw;
    memcpy(shadow->floor.xyz,c->motion.xyz,sizeof shadow->floor.xyz);
    bfm_bedroom_travel_air_shadow(a->scene,a->scene_bytes,a->room_address,&shadow->floor);
    return 1;
}

int bfm_village_mine_corridor_387_shadow_draw(const BfmVillageMineCorridor387 *a,
    const BfmMineCreatureShadow *shadow, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!a || !shadow || !figure || !view) return 0;
    if (!shadow->active || !shadow->floor.shadow_visible) return 1;
    if (shadow->parent != BFM_MINE_WEST_CREATURE && shadow->parent >= a->row_count) return 0;
    return bfm_bedroom_figure_add_floor_shadow(figure,view,&shadow->floor,
        a->shadow_texture,4096,shadow->at_birth);
}

int bfm_village_mine_corridor_387_draw_body(const BfmVillageMineCorridor387 *a, unsigned parent,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    const MineResident *c = shadow_parent(a,parent);
    if (!c || !figure || !view) return 0;
    if (!c->allocated) return 1;
    if (c->hidden) return draw_halo(a,c,figure,view);
    const BfmForestPose *pose = c->cut_pose_active ? &a->cut_pose : &a->pose;
    BfmForestActor actor = {.mesh=a->mesh,.pose=pose,.root=&c->submitted,.visible_parts=1};
    if (!bfm_bedroom_figure_lights(figure,a->lamps)) return 0;
    int ok = bfm_bedroom_figure_add_actor(figure,view,&actor,NULL,NULL,0);
    return bfm_bedroom_figure_lights(figure,a->field_lamps) && ok && draw_halo(a,c,figure,view);
}

static int draw_with_shadow(const BfmVillageMineCorridor387 *a, unsigned parent,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    const MineResident *c = shadow_parent(a,parent);
    if (!c || !figure || !view) return 0;
    if (!c->allocated) return 1;
    if (c->hidden) return draw_halo(a,c,figure,view);
    BfmMineCreatureShadow shadow;
    return bfm_village_mine_corridor_387_shadow_start(a,parent,&shadow) &&
           bfm_village_mine_corridor_387_shadow_step(a,&shadow) &&
           bfm_village_mine_corridor_387_shadow_draw(a,&shadow,figure,view) &&
           bfm_village_mine_corridor_387_draw_body(a,parent,figure,view);
}

int bfm_village_mine_corridor_387_active(const BfmVillageMineCorridor387 *a)
{
    return a && a->fixed.allocated;
}

int bfm_village_mine_corridor_387_draw(const BfmVillageMineCorridor387 *a,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    return draw_with_shadow(a,BFM_MINE_WEST_CREATURE,figure,view);
}

int bfm_village_mine_corridor_387_draw_row(const BfmVillageMineCorridor387 *a, unsigned row,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!a || !figure || !view || row >= a->row_count) return 0;
    return draw_with_shadow(a,row,figure,view);
}
