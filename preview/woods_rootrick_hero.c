/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_hero.h"
#include "field_waypoint.h"
#include "forest_motion.h"

#include <limits.h>
#include <string.h>

enum { Q = 65536, CENTER_X = -224, APPROACH_Z = 1024, TAKEOFF_Z = 978 };

static int64_t whole(int64_t value)
{
    return value / Q - (value % Q < 0);
}

static void project(int64_t position[3], const int16_t point[3])
{
    for (unsigned i = 0; i < 3; ++i)
        position[i] += ((int64_t)point[i] - whole(position[i])) * Q;
}

static int extract(const int64_t position[3], int16_t point[3])
{
    for (unsigned i = 0; i < 3; ++i) {
        int64_t v = whole(position[i]);
        if (v < INT16_MIN || v > INT16_MAX) return 0;
        point[i] = (int16_t)v;
    }
    return 1;
}

static void forward(unsigned yaw, int32_t speed, int32_t *x, int32_t *z)
{
    ForestPlayerState step = {0};
    step.field_2c.word = -speed;
    bfm_forest_level_velocity(&step, 0, (int16_t)yaw, 0);
    *x = (int32_t)step.velocity[0];
    *z = (int32_t)step.velocity[2];
}

static int wall_grip(const BfmVillageGround *ground, const int16_t foot[3], unsigned yaw)
{
    int32_t x, z;
    forward(yaw, -7 * Q, &x, &z);
    int16_t from[3] = {foot[0] + whole(x), foot[1], foot[2] + whole(z)};
    forward(yaw, 24 * Q, &x, &z);
    int16_t to[3] = {foot[0] + whole(x), foot[1], foot[2] + whole(z)};
    uint16_t material = 0;
    return (bfm_village_ground_support_probe(ground, 1, from, to, &material) & 0x8000u)
        && (material & 255) == 21;
}

static void center_player(int16_t foot[3])
{
    int delta = CENTER_X - foot[0];
    foot[0] += delta / 10 ? delta / 10 : (delta > 0) - (delta < 0);
}

/* All three mounted type-176 bodies have unit, unrotated frames. */
static int contains(const BfmRootrickHeroObstacle *obstacle, const int16_t point[3])
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        int rel = point[axis] - obstacle->transform.translation[axis];
        if (rel < obstacle->box.low[axis] || rel > obstacle->box.high[axis]) return 0;
    }
    return 1;
}

static int player_body(const BfmRootrickHeroObstacle solids[3], unsigned rider,
                       const int16_t from[3], int16_t to[3], unsigned clock, int test_only)
{
    int16_t old[3] = {from[0], from[1]-16, from[2]};
    int16_t end[3] = {to[0], to[1]-16, to[2]};
    int touched = 0;
    for (unsigned i = 0; i < 3; ++i) {
        const BfmRootrickHeroObstacle *s = &solids[i];
        if (!s->present || !(s->body_flags & 0x1800u) || rider == i+1) continue;
        int16_t center[3], contact[3], normal[3];
        if (!bfm_forest_guard_contact_point(&s->box, &s->transform, old, end,
                                             center, contact, normal)) continue;
        if (test_only) return 1;
        touched = 1;
        int inside = contains(s,old);
        if (!inside) {
            if (!bfm_forest_body_slide(&s->box,&s->transform,old,end,contact)) continue;
            memcpy(end,contact,sizeof end);
            to[0] = end[0]; to[1] = end[1]+16; to[2] = end[2];
        }
        /* In this room both type-176 shapes also eject the chest sensor
         * horizontally when an outside sweep hits their top or underside. */
        if (inside || normal[1] > 2048 || normal[1] < -2048) {
            unsigned quadrant = (clock >> 5) & 3;
            int dx = quadrant < 2 ? 7 : 8, dz = quadrant == 2 ? 7 : 8;
            to[0] = contact[0] + (contact[0] > center[0] ? dx : -dx);
            to[2] = contact[2] + (contact[2] > center[2] ? dz : -dz);
        }
    }
    return touched;
}

static unsigned platform_support(const BfmRootrickHeroObstacle solids[3],
    unsigned rider, const int16_t previous[3], int16_t next[3])
{
    int16_t from[3], to[3];
    memcpy(from,previous,sizeof from); memcpy(to,next,sizeof to);
    if (rider || previous[1] == next[1]) {
        from[1] -= 3;
        to[1] += rider || memcmp(previous,next,sizeof from) ? 8 : 5;
    }
    for (unsigned i=0; i<3; ++i) {
        const BfmRootrickHeroObstacle *s = &solids[i];
        if ((rider && rider != i+1) || !s->present || !(s->body_flags & 0x0400) ||
            s->transform.translation[1] < next[1]) continue;
        int16_t end[3];
        if (bfm_forest_body_slide(&s->box,&s->transform,from,to,end)) {
            memcpy(next,end,sizeof end);
            return i+1;
        }
    }
    return 0;
}

int bfm_rootrick_hero_carry_after(BfmRootrickHero *h, const BfmVillageGround *ground,
    const BfmRootrickHeroObstacle solids[3], int64_t position[3])
{
    if (!h || !ground || !solids || !position || h->rider > 3) return 0;
    unsigned mask = h->previous_mask;
    h->previous_mask = 0;
    if (!h->rider || !mask) return 1;
    unsigned i = h->rider - 1;
    if (!solids[i].present || !(mask & (1u << i))) return 1;

    /* The room's three platforms translate only along Z. Their actor step
     * transforms the newly resolved player position, even on the field he
     * first becomes a rider. A wall blocks the carry but keeps his footing. */
    int dz = solids[i].transform.translation[2] - h->previous_z[i];
    if (!dz) return 1;
    int16_t start[3];
    if (!extract(position,start)) return 0;
    int z = start[2] + dz;
    if (z < INT16_MIN || z > INT16_MAX) return 0;
    int16_t end[3] = {start[0],start[1],(int16_t)z};
    uint16_t material = 0;
    if (!(bfm_village_ground_support_probe(ground,1,start,end,&material) & 0x8000))
        position[2] += (int64_t)dz * Q;
    return 1;
}

void bfm_rootrick_hero_begin(BfmRootrickHero *h)
{
    if (h) *h = (BfmRootrickHero){.grounded=1};
}

int bfm_rootrick_hero_standing(const BfmRootrickHero *h)
{
    return h && h->grounded && !h->walking;
}

static int advance_ground(BfmRootrickHero *h, const BfmVillageGround *ground,
    const BfmRootrickHeroObstacle solids[3], int64_t position[3],
    BfmBedroomRoot *root, int16_t end[3])
{
    const int16_t *from = h->previous_foot;
    if (h->rider) {
        unsigned rider = platform_support(solids,h->rider,from,end);
        if (rider) {
            project(position,end);
            return 1;
        }
        h->rider = 0;
    }
    BfmForestContact floor = {0};
    BfmVillageGroundResult result = bfm_village_ground_step(ground,from,end,&floor);
    if (result == BFM_VILLAGE_GROUND_BLOCKED) return 0;
    h->grounded = result == BFM_VILLAGE_GROUND_FOOTING;
    if (h->grounded) {
        memcpy(root->normal,floor.normal,sizeof root->normal);
    } else {
        /* A moving floor can retract from under a new staging walk. */
        h->phase = BFM_ROOTRICK_HERO_VAULT;
        h->kick_ticks = 0;
        h->forward_speed = h->vertical_speed = 0;
    }
    project(position,end);
    return 1;
}

static int approach(BfmRootrickHero *h, const BfmVillageGround *ground,
    const BfmRootrickHeroObstacle solids[3], int camera, int64_t position[3],
    BfmBedroomRoot *root, BfmBedroomAction *action)
{
    int runup = h->phase == BFM_ROOTRICK_HERO_RUNUP;
    int z = runup ? TAKEOFF_Z : APPROACH_Z;
    int dx = CENTER_X-root->xyz[0], dz = z-root->xyz[2];
    if ((int64_t)dx*dx+(int64_t)dz*dz < 16*16) {
        int16_t endpoint[3] = {CENTER_X,root->xyz[1],z};
        if (!advance_ground(h,ground,solids,position,root,endpoint)) return 0;
        if (!h->grounded) { *action = BFM_BEDROOM_FALL; return 1; }
        h->walking = 0;
        if (runup) {
            h->phase = BFM_ROOTRICK_HERO_JUMP;
            h->grounded = 0;
            h->rider = 0;
            h->vertical_speed = -18 * Q;
            h->forward_speed = 9*Q-Q/16;
            *action = BFM_BEDROOM_FALL;
        } else {
            h->phase = BFM_ROOTRICK_HERO_RUNUP;
            *action = BFM_BEDROOM_STAND;
        }
        return 1;
    }
    BfmWaypointInput steer = bfm_waypoint_input(dx,dz,0,camera);
    root->yaw = steer.yaw;
    if (!h->walking) {
        h->walking = 1;
        *action = BFM_BEDROOM_WALK;
        return 1;
    }
    int32_t vx, vz;
    forward(root->yaw,steer.speed_q16,&vx,&vz);
    position[0] += vx;
    position[2] += vz;
    int16_t end[3];
    if (!extract(position,end)) return 0;
    if (runup) center_player(end);
    if (!advance_ground(h,ground,solids,position,root,end)) return 0;
    if (!h->grounded) *action = BFM_BEDROOM_FALL;
    return 1;
}

static void take_vault(BfmRootrickHero *h)
{
    h->phase = BFM_ROOTRICK_HERO_VAULT;
    h->attached = h->grounded = 0;
    h->cooldown = 10;
    h->kick_ticks = 16;
    h->vertical_speed = -24*Q;
    h->forward_speed = 6*Q;
}

static int grip_field(BfmRootrickHero *h, BfmBedroomMotion *motion, const BfmVillageGround *ground,
    const BfmRootrickHeroObstacle solids[3], BfmFieldInput input, unsigned clock,
    int64_t position[3], BfmBedroomRoot *root, BfmBedroomPose *pose)
{
    int dy;
    if (!bfm_rootrick_grip_step(&h->grip,motion,input,pose,&dy)) return 0;
    int16_t end[3] = {root->xyz[0],root->xyz[1]+dy,root->xyz[2]};
    if (player_body(solids,0,h->previous_foot,end,clock,1))
        memcpy(end,root->xyz,sizeof end);
    if (!wall_grip(ground,end,root->yaw)) {
        take_vault(h);
        if (!bfm_bedroom_motion_reenter_fall(motion,1)) return 0;
    } else {
        player_body(solids,0,h->previous_foot,end,clock,0);
        uint16_t material = 0;
        bfm_village_ground_support_probe(ground,1,h->previous_foot,end,&material);
    }
    project(position,end);
    return 1;
}

static int airborne(BfmRootrickHero *h, BfmBedroomMotion *motion, const BfmVillageGround *ground,
    const BfmRootrickHeroObstacle solids[3], int camera, unsigned clock, int can_climb,
    int64_t position[3], BfmBedroomRoot *root, BfmBedroomPose *pose)
{
    int jumping = h->phase == BFM_ROOTRICK_HERO_JUMP;
    if (jumping) {
        BfmWaypointInput forward_pad = bfm_waypoint_input(0,-128,0,camera);
        root->yaw = forward_pad.yaw;
        h->forward_speed += Q/16;
        if (h->forward_speed > 11*Q+5*Q/8) h->forward_speed = 11*Q+5*Q/8;
    } else if (h->kick_ticks && !--h->kick_ticks) h->forward_speed = 0;
    h->vertical_speed += !jumping && h->vertical_speed < 0 ? 21*Q/8 : 13*Q/8;
    if (h->vertical_speed > 31*Q) h->vertical_speed = 31*Q;

    int32_t vx,vz;
    forward(root->yaw,h->forward_speed,&vx,&vz);
    position[0] += vx; position[1] += h->vertical_speed; position[2] += vz;
    int16_t end[3];
    if (!extract(position,end)) return 0;
    center_player(end);
    player_body(solids,0,h->previous_foot,end,clock,0);
    uint16_t wall = 0, reason = 0;
    BfmForestContact floor = {0};
    BfmVillageGroundResult result = bfm_village_ground_bank_step(ground,h->previous_foot,end,1,
                                                                 &floor,&wall,&reason);
    if (result == BFM_VILLAGE_GROUND_BLOCKED) return 0;
    if (h->vertical_speed > 0 && result != BFM_VILLAGE_GROUND_FOOTING) {
        h->rider = platform_support(solids,0,h->previous_foot,end);
        if (h->rider) {
            result = BFM_VILLAGE_GROUND_FOOTING;
            floor.normal[1] = -4096;
        }
    }
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    if (result == BFM_VILLAGE_GROUND_FOOTING && floor.slope != 0x8000) {
        h->phase = BFM_ROOTRICK_HERO_LANDING;
        h->grounded = 0;
        h->attached = 0;
        /* Contact stops translation. The next unassisted fall resumes
         * gravity from the speed at which Musashi reached this support. */
        h->forward_speed = 0;
        memcpy(root->normal,floor.normal,sizeof root->normal);
        action = BFM_BEDROOM_LAND;
    } else if ((wall & 255) == 21 && can_climb && !h->cooldown) {
        /* Derive the facing from the actual contacted wall's outward normal. */
        int16_t hit[3],normal[3]; uint16_t material = 0;
        if (!extract(position,hit)) return 0;
        if (bfm_village_ground_blade(ground,h->previous_foot,hit,normal,&material)
            && (material & 255) == 21)
            root->yaw = (unsigned)bfm_waypoint_heading(normal[0],normal[2]);
        if (!bfm_rootrick_grip_begin(&h->grip,motion)) return 0;
        h->phase = BFM_ROOTRICK_HERO_GRIP;
        h->attached = 1;
        h->vertical_speed = h->forward_speed = 0;
    }
    project(position,end);
    if (!bfm_bedroom_motion_step(motion,action,pose)) return 0;
    if (h->vertical_speed >= 0 && h->phase == BFM_ROOTRICK_HERO_VAULT)
        bfm_bedroom_motion_descend(motion);
    return 1;
}

int bfm_rootrick_hero_step(BfmRootrickHero *h, BfmBedroomMotion *motion,
    const BfmVillageGround *ground, const BfmRootrickHeroObstacle obstacles[3],
    BfmFieldInput input, int camera_yaw, unsigned field_tick, int can_climb,
    int64_t position[3], BfmBedroomRoot *root, BfmBedroomPose *pose)
{
    if (!h || !motion || !ground || !obstacles || !position || !root || !pose) return 0;
    /* Contact sweeps start at the last submitted feet. Platform motion may
     * already have carried the physical player beyond that point. */
    memcpy(h->previous_foot,root->xyz,sizeof h->previous_foot);
    if (!extract(position,root->xyz)) return 0;
    h->previous_mask = 0;
    for (unsigned i = 0; i < 3; ++i) {
        if (obstacles[i].present) {
            h->previous_z[i] = (int16_t)obstacles[i].transform.translation[2];
            h->previous_mask |= 1u<<i;
        }
    }
    if (h->cooldown) --h->cooldown;
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    switch (h->phase) {
    case BFM_ROOTRICK_HERO_APPROACH:
    case BFM_ROOTRICK_HERO_RUNUP:
        if (!approach(h,ground,obstacles,camera_yaw,position,root,&action) ||
            !bfm_bedroom_motion_step(motion,action,pose)) return 0;
        break;
    case BFM_ROOTRICK_HERO_JUMP:
    case BFM_ROOTRICK_HERO_VAULT:
        if (!airborne(h,motion,ground,obstacles,camera_yaw,field_tick,can_climb,
                      position,root,pose)) return 0;
        break;
    case BFM_ROOTRICK_HERO_GRIP:
        if (!grip_field(h,motion,ground,obstacles,input,field_tick,position,root,pose)) return 0;
        break;
    case BFM_ROOTRICK_HERO_LANDING:
        /* The standing signal is raised after the landing animation ends.
         * Until then the script centers Musashi while the support carries him. */
        if (!bfm_bedroom_motion_step(motion,BFM_BEDROOM_NONE,pose)) return 0;
        if (!h->grounded) {
            int16_t centered[3];
            memcpy(centered,root->xyz,sizeof centered);
            center_player(centered);
            project(position,centered);
            if (bfm_bedroom_motion_marker(motion) != BFM_BODY_END) break;
            h->grounded = 1;
            if (!bfm_bedroom_motion_reenter_stance(motion,-1)) return 0;
        }
        if (root->xyz[1] >= -511) h->phase = BFM_ROOTRICK_HERO_RETRY;
        else if (h->rider) { h->phase = BFM_ROOTRICK_HERO_APPROACH; h->walking = 0; }
        break;
    case BFM_ROOTRICK_HERO_RETRY:
        if (!bfm_bedroom_motion_step(motion,BFM_BEDROOM_NONE,pose)) return 0;
        break;
    }
    return extract(position,root->xyz);
}
