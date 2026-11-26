/* SPDX-License-Identifier: MIT */
#include "tower_summit_residents.h"
#include "forest_guard_clip.h"
#include "forest_opening_data.h"
#include "forest_guard_contact.h"
#include "forest_pose.h"
#include "forest_rifles.h"
#include "forest_rifle_contact.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    BfmForestShoreMotion motion;
    BfmForestTransform root;
    BfmForestPose pose;
    uint32_t generation;
    int bound, tether_meter, tether_width, returning, striking;
    unsigned health, stun_timer, stun_frame, turn_timer;
    int64_t prior_radius2;
} Resident;

struct BfmTowerSummitResidents {
    const BfmForestField *room;
    MusashiForestPlayerMesh *mesh;
    BfmForestGuardClip clip[BFM_SHORE_FOLLOW_THROUGH + 1], stunned;
    BfmForestGuardBox box, obstruction;
    int16_t origin[3], pursuit_point[3];
    uint32_t generation;
    unsigned limit, count, clock, health, reward;
    Resident member[BFM_SUMMIT_RESIDENTS];
};

static int read_box(const BfmForestField *room, uint32_t address, BfmForestGuardBox *out)
{
    const uint8_t *box = bfm_forest_room_bytes(room, address, 16);
    if (!box || u32le(box)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        out->low[axis] = s16le(box + 4 + 4 * axis);
        out->high[axis] = s16le(box + 6 + 4 * axis);
        if (out->low[axis] > out->high[axis]) return 0;
    }
    return 1;
}

BfmTowerSummitResidents *bfm_tower_summit_residents_open(const BfmForestField *room)
{
    const uint8_t *list = bfm_forest_room_bytes(room, UINT32_C(0x801a7be4), 4);
    const uint8_t *manager = list ? bfm_forest_room_bytes(room, u32le(list), 20) : NULL;
    const uint8_t *profile = bfm_forest_room_bytes(room, UINT32_C(0x8018e890), 52);
    const uint8_t *limit = bfm_forest_room_bytes(room, UINT32_C(0x8018f2f0), 4);
    const uint8_t *origin = bfm_forest_room_bytes(room, UINT32_C(0x8018e6cc), 6);
    const uint8_t *pursuit = bfm_forest_room_bytes(room, UINT32_C(0x8018e6e4), 6);
    if (!manager || u16le(manager + 6) != 53 || !profile || !limit || !origin || !pursuit ||
        !u32le(limit) || u32le(limit) > BFM_SUMMIT_RESIDENTS || s16le(profile) <= 0) return NULL;
    BfmTowerSummitResidents *p = calloc(1, sizeof *p);
    if (!p) return NULL;
    p->room = room;
    if (!read_box(room, u32le(profile + 8), &p->box) ||
        !read_box(room, UINT32_C(0x8018e980), &p->obstruction)) goto fail;
    p->health = u16le(profile);
    p->reward = u16le(profile + 44);
    p->limit = u32le(limit);
    p->mesh = bfm_forest_rifle_mesh(room, u32le(profile + 4), BFM_GUARD_PARTS);
    if (!p->mesh) goto fail;
    for (unsigned axis = 0; axis < 3; ++axis) {
        p->origin[axis] = s16le(origin + 2 * axis);
        p->pursuit_point[axis] = s16le(pursuit + 2 * axis);
    }
    static const struct { BfmForestShorePhase phase; uint32_t keys; } clips[] = {
        {BFM_SHORE_WALK, UINT32_C(0x801a648c)}, {BFM_SHORE_NOTICE, UINT32_C(0x801a65bc)},
        {BFM_SHORE_PURSUIT, UINT32_C(0x801a6534)}, {BFM_SHORE_STRIKE, UINT32_C(0x801a668c)},
        {BFM_SHORE_PAUSE, UINT32_C(0x801a629c)}, {BFM_SHORE_FOLLOW_THROUGH, UINT32_C(0x801a6754)}
    };
    for (unsigned i = 0; i < sizeof clips / sizeof clips[0]; ++i)
        if (!bfm_forest_guard_clip_read(room, clips[i].keys, 0xffff, &p->clip[clips[i].phase])) goto fail;
    p->clip[BFM_SHORE_TURN] = p->clip[BFM_SHORE_WALK];
    if (!bfm_forest_guard_clip_read(room, UINT32_C(0x801a6e7c), 0xffff, &p->stunned)) goto fail;
    return p;
fail:
    bfm_tower_summit_residents_free(p);
    return NULL;
}

void bfm_tower_summit_residents_free(BfmTowerSummitResidents *p)
{
    if (p) { musashi_forest_player_mesh_free(p->mesh); free(p); }
}

static int publish(BfmTowerSummitResidents *p, Resident *r)
{
    const BfmForestGuardClip *clip = r->stun_timer ? &p->stunned : &p->clip[r->motion.phase];
    unsigned key = r->stun_timer ? r->stun_frame : r->motion.frame;
    if (key >= clip->count) return 0;
    bfm_forest_actor_root(r->motion.yaw, r->motion.position, &r->root);
    if (r->bound) {
        for (unsigned row = 0; row < 3; ++row) {
            for (unsigned axis = 0; axis < 3; axis += 2) {
                int v = r->root.rotation[row][axis] * r->tether_width;
                r->root.rotation[row][axis] = v / 4096 - (v % 4096 < 0);
            }
        }
    }
    return bfm_forest_actor_pose_decode(clip->pose[key], BFM_GUARD_POSE_BYTES, BFM_GUARD_PARTS, &r->pose);
}

static void patrol(Resident *r, BfmGameplayRun *run)
{
    BfmForestShoreMotion *m = &r->motion;
    m->phase = BFM_SHORE_WALK;
    m->frame = m->walk_updates = 0;
    m->notice_delay = 30;
    m->turn_target = m->travel_yaw = m->yaw;
    m->travel_speed = 4;
    m->patrol_timer = 70 + (bfm_gameplay_random(run) & 31);
}

static unsigned turn(unsigned yaw, unsigned target, int divisor)
{
    int d = (int)target - (int)yaw;
    if (d > 2048) d -= 4096;
    if (d < -2048) d += 4096;
    int step = d / divisor;
    if (!step && d) step = d < 0 ? -1 : 1;
    return (yaw + step) & 4095;
}

static int spawn(BfmTowerSummitResidents *p, unsigned slot, const int16_t hero[3], BfmGameplayRun *run)
{
    Resident *r = &p->member[slot];
    BfmForestShoreMotion *m = &r->motion;
    unsigned away = bfm_forest_shore_bearing(p->origin[0] - hero[0], p->origin[2] - hero[2]);
    /* The bearing helper maps a zero-length vector to zero. The spawn service
     * derives its heading from the original angle's quarter-turn basis. */
    if (hero[0] == p->origin[0] && hero[2] == p->origin[2]) away = 3072;
    BfmForestTransform ring;
    bfm_forest_actor_root(away, p->origin, &ring);
    for (unsigned axis = 0; axis < 3; axis += 2) {
        int v = 272 * ring.rotation[axis][2];
        m->position[axis] = (int16_t)(v / 4096 - (v % 4096 < 0));
    }
    BfmForestContact floor;
    if (!bfm_forest_ground(p->room, m->position[0], -4100, m->position[2], &floor)) return 0;
    m->position[1] = floor.height;
    m->y_q16 = (int32_t)floor.height * 65536;
    m->yaw = bfm_gameplay_random(run) & 4095;
    patrol(r, run);
    r->health = p->health;
    r->generation = ++p->generation;
    if (!publish(p, r)) return 0;
    ++p->count;
    return 1;
}

static void separate(BfmTowerSummitResidents *p, Resident *r)
{
    for (unsigned i = 0; i < p->limit; ++i) {
        const Resident *other = &p->member[i];
        if (other == r || !other->health) continue;
        int16_t *at = r->motion.position;
        const int16_t *near = other->motion.position;
        int dx = at[0] - near[0], dy = at[1] - near[1], dz = at[2] - near[2];
        if ((int64_t)dx*dx + (int64_t)dy*dy + (int64_t)dz*dz >= 64*64) continue;
        unsigned bearing = dx || dz ? bfm_forest_shore_bearing(dx, dz) : 3072;
        BfmForestTransform offset;
        bfm_forest_actor_root(bearing, near, &offset);
        int16_t next[3] = {near[0], near[1], near[2]};
        for (unsigned axis = 0; axis < 3; axis += 2) {
            int v = 65 * offset.rotation[axis][2];
            next[axis] += v / 4096 - (v % 4096 < 0);
        }
        BfmForestContact floor;
        if (bfm_forest_ground(p->room, next[0], next[1] - 4, next[2], &floor)) {
            next[1] = floor.height;
            memcpy(at, next, sizeof next);
        }
        break;
    }
}

/* Beyond the eastern approach the guard walks back across the boundary.
 * The rest of the ring uses the stationary turn. */
static int return_to_ring(BfmTowerSummitResidents *p, Resident *r, BfmGameplayRun *run)
{
    BfmForestShoreMotion *m = &r->motion;
    BfmForestTransform facing;
    m->yaw = m->turn_target;
    bfm_forest_actor_root(m->yaw, m->position, &facing);
    int dx = m->x_fraction - 128 * facing.rotation[0][2];
    int dz = m->z_fraction - 128 * facing.rotation[2][2];
    int16_t to[3] = { m->position[0] + (dx - (uint16_t)dx) / 65536, m->position[1] + 1,
                      m->position[2] + (dz - (uint16_t)dz) / 65536 };
    BfmForestShoreFloor floor = bfm_forest_pursuit_floor_step(p->room, m->position, to);
    if (floor == BFM_SHORE_FLOOR_FLAT || floor == BFM_SHORE_FLOOR_GRADE) {
        memcpy(m->position, to, sizeof to);
        m->x_fraction = (uint16_t)dx;
        m->z_fraction = (uint16_t)dz;
        separate(p, r);
    }
    m->frame = (m->frame + 1) % p->clip[BFM_SHORE_WALK].count;
    if (m->position[0] < 504) {
        r->returning = 0;
        patrol(r, run);
        m->notice_delay = bfm_gameplay_random(run) & 31;
        m->patrol_timer = bfm_gameplay_random(run) & 31;
    }
    return publish(p, r);
}

static int update(BfmTowerSummitResidents *p, Resident *r, const int16_t hero[3],
                  int pickup, BfmGameplayRun *run)
{
    BfmForestShoreMotion *m = &r->motion;
    r->striking = 0;
    if (r->bound) {
        int width = (144 - r->tether_meter) * 4096 / 144;
        if (!r->tether_meter) r->tether_width = 4096;
        r->tether_width += (width - r->tether_width) / 4;
        return publish(p, r);
    }
    if (r->stun_timer) {
        r->stun_frame = (r->stun_frame + 1) % p->stunned.count;
        if (!pickup && !--r->stun_timer) patrol(r, run);
        return publish(p, r);
    }
    if (r->returning) return return_to_ring(p, r, run);
    if (r->turn_timer) {
        m->yaw = turn(m->yaw, m->turn_target, 4);
        m->frame = (m->frame + 1) % p->clip[BFM_SHORE_WALK].count;
        if (!--r->turn_timer) patrol(r, run);
        return publish(p, r);
    }
    BfmForestShorePhase prior = m->phase;
    if (prior == BFM_SHORE_WALK || prior == BFM_SHORE_PURSUIT) {
        int dx = m->position[0] - p->origin[0], dy = m->position[1] - p->origin[1];
        int dz = m->position[2] - p->origin[2];
        int64_t radius2 = (int64_t)dx*dx + (int64_t)dy*dy + (int64_t)dz*dz;
        unsigned inward = bfm_forest_shore_bearing(dx, dz);
        int outer = prior == BFM_SHORE_PURSUIT ? 576 : 496;
        if (prior == BFM_SHORE_PURSUIT && hero[0] >= 736 && inward > 896 && inward < 1152)
            outer = 496;
        if (radius2 > outer*outer && radius2 > r->prior_radius2) {
            int spread = m->position[0] > 512 ? 0 : (int)(bfm_gameplay_random(run) & 2047) - 1024;
            patrol(r, run);
            m->turn_target = (inward + spread) & 4095;
            m->notice_delay = 30 + (bfm_gameplay_random(run) & 31);
            r->returning = m->position[0] > 512;
            r->turn_timer = r->returning ? 0 : 16;
            r->prior_radius2 = radius2;
            return publish(p, r);
        }
        r->prior_radius2 = radius2;
    }
    unsigned yaw = m->yaw, notice = m->notice_delay;
    if (pickup && prior == BFM_SHORE_WALK && notice < 2) m->notice_delay = 2;
    r->striking = (prior == BFM_SHORE_STRIKE && m->patrol_timer == 7) ||
                  (prior == BFM_SHORE_FOLLOW_THROUGH && m->patrol_timer == 10);
    int moved = bfm_forest_shore_tick(m, p->room, hero, p->clip[prior].count, run);
    if ((!moved || m->phase == BFM_SHORE_TURN) &&
        (prior == BFM_SHORE_WALK || prior == BFM_SHORE_PURSUIT)) {
        /* An unsupported footstep restores the last grounded root, then
         * the guard turns around. Other live room actors continue. */
        patrol(r, run);
        m->turn_target = (yaw + 2048) & 4095;
        m->notice_delay = 30 + (bfm_gameplay_random(run) & 31);
        r->turn_timer = 22;
        return publish(p, r);
    }
    if (!moved) return 0;
    if (pickup && prior == BFM_SHORE_WALK && notice < 2) m->notice_delay = 0;
    if (prior == BFM_SHORE_WALK || prior == BFM_SHORE_PURSUIT) separate(p, r);
    /* On the sword approach these guards steer toward the mounted bridge
     * point. The attack still measures the distance to the actual player. */
    if (prior == BFM_SHORE_PURSUIT && hero[0] >= 544) {
        unsigned bearing = bfm_forest_shore_bearing(m->position[0] - p->pursuit_point[0],
                                                   m->position[2] - p->pursuit_point[2]);
        m->yaw = turn(yaw, bearing, 6);
    }
    if (prior != BFM_SHORE_WALK && m->phase == BFM_SHORE_WALK) patrol(r, run);
    return publish(p, r);
}

int bfm_tower_summit_residents_tick(BfmTowerSummitResidents *p, const int16_t player[3],
                                    int scripted, int pickup, BfmGameplayRun *run)
{
    if (!p || !player || !run) return 0;
    if (p->clock < 60) ++p->clock;
    if (!pickup && !scripted && p->clock == 60 && p->count < p->limit) {
        unsigned slot = 0;
        while (slot < p->limit && p->member[slot].health) ++slot;
        if (slot == p->limit || !spawn(p, slot, player, run)) return 0;
        p->clock = 0;
    }
    for (unsigned i = 0; i < p->limit; ++i)
        if (p->member[i].health && !update(p, &p->member[i], player, pickup, run)) return 0;
    return 1;
}

void bfm_tower_summit_residents_pause(BfmTowerSummitResidents *p)
{
    if (!p) return;
    for (unsigned i = 0; i < p->limit; ++i)
        p->member[i].striking = 0;
}

static unsigned state(const Resident *r)
{
    if (r->bound) return 4;
    if (r->stun_timer) return 8;
    return r->motion.phase >= BFM_SHORE_STRIKE ? 2 : 1;
}

int bfm_tower_summit_residents_get(const BfmTowerSummitResidents *p, unsigned slot,
                                   BfmTowerSummitResident *out)
{
    if (!p || !out || slot >= p->limit || !p->member[slot].health) return 0;
    const Resident *r = &p->member[slot];
    memcpy(out->position, r->motion.position, sizeof out->position);
    out->state = state(r);
    out->health = r->health;
    out->reward = p->reward;
    out->stun_ticks = r->stun_timer;
    out->generation = r->generation;
    out->variant = 0x400;
    out->profile = UINT32_C(0x8018e890);
    return 1;
}

size_t bfm_tower_summit_residents_actors(const BfmTowerSummitResidents *p,
                                        BfmForestActor *out, size_t capacity)
{
    if (!p || !out || capacity < p->count) return 0;
    size_t count = 0;
    for (unsigned i = 0; i < p->limit; ++i) {
        const Resident *r = &p->member[i];
        if (r->health) out[count++] = (BfmForestActor){.mesh = p->mesh, .pose = &r->pose,
            .root = &r->root, .visible_parts = (1u << BFM_GUARD_PARTS) - 1};
    }
    return count;
}

int bfm_tower_summit_residents_contact(const BfmTowerSummitResidents *p, unsigned slot,
                                      const int16_t from[3], const int16_t to[3],
                                      int16_t center[3], int16_t point[3])
{
    return p && slot < p->limit && p->member[slot].health && !p->member[slot].bound && from && to && center &&
           bfm_forest_guard_contact_point(&p->box, &p->member[slot].root, from, to, center, point, NULL);
}

int bfm_tower_summit_residents_slide(const BfmTowerSummitResidents *p,
                                    const int16_t from[3], const int16_t to[3],
                                    int16_t contact[3])
{
    if (!p || !from || !to || !contact) return 0;
    int16_t end[3] = {to[0], to[1], to[2]};
    int collided = 0;
    for (unsigned pass = 0; pass < 2; ++pass) {
        int adjusted = 0;
        for (unsigned slot = 0; slot < p->limit; ++slot) {
            const Resident *r = &p->member[slot];
            if (r->health && !r->bound &&
                bfm_forest_body_slide_on_entry(&p->obstruction, &r->root, from, end, end))
                adjusted = 1;
        }
        if (!adjusted) break;
        collided = 1;
    }
    if (collided) memcpy(contact, end, sizeof end);
    return collided;
}

unsigned bfm_tower_summit_residents_strike(const BfmTowerSummitResidents *p,
                                          const BfmForestTransform *player, int16_t *yaw)
{
    if (!p || !player || !yaw) return 0;
    for (unsigned i = 0; i < p->limit; ++i) {
        const Resident *r = &p->member[i];
        if (r->health && r->striking && !r->bound && !r->stun_timer &&
            bfm_forest_attack_segment_hits_player(p->room, UINT32_C(0x8018e6d4), UINT32_C(0x8018c5ac),
                                                   &r->root, player)) {
            *yaw = (int16_t)r->motion.yaw;
            return 11;
        }
    }
    return 0;
}

static Resident *member(BfmTowerSummitResidents *p, unsigned slot, uint32_t generation)
{
    if (!p || slot >= p->limit) return NULL;
    Resident *r = &p->member[slot];
    return r->health && r->generation == generation ? r : NULL;
}

int bfm_tower_summit_residents_stun(BfmTowerSummitResidents *p, unsigned slot, uint32_t generation)
{
    Resident *r = member(p, slot, generation);
    if (!r || r->bound || r->health <= 1) return 0;
    --r->health;
    r->stun_frame = r->turn_timer = r->returning = 0;
    r->stun_timer = 120;
    return publish(p, r);
}

int bfm_tower_summit_residents_bind(BfmTowerSummitResidents *p, unsigned slot, uint32_t generation)
{
    Resident *r = member(p, slot, generation);
    if (!r || r->bound) return 0;
    r->bound = 1;
    r->tether_meter = 0;
    r->tether_width = 4096;
    return 1;
}

int bfm_tower_summit_residents_hold(BfmTowerSummitResidents *p, unsigned slot,
                                    uint32_t generation, unsigned meter)
{
    Resident *r = member(p, slot, generation);
    if (!r || !r->bound || meter > 128) return 0;
    r->tether_meter = (int)meter;
    return 1;
}

int bfm_tower_summit_residents_release(BfmTowerSummitResidents *p, unsigned slot, uint32_t generation,
                                       BfmGameplayRun *run)
{
    Resident *r = member(p, slot, generation);
    if (!r || !r->bound || !run) return 0;
    r->bound = 0;
    r->stun_timer = r->turn_timer = r->returning = 0;
    patrol(r, run);
    return publish(p, r);
}

int bfm_tower_summit_residents_consume(BfmTowerSummitResidents *p, unsigned slot, uint32_t generation,
                                       unsigned *reward)
{
    Resident *r = member(p, slot, generation);
    if (!r || !r->bound || !reward) return 0;
    *reward = p->reward;
    *r = (Resident){0};
    --p->count;
    return 1;
}
