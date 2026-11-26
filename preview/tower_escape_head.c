/* SPDX-License-Identifier: MIT */
#include "tower_escape_head.h"
#include "tower_escape.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

enum { Q = 65536, CRUISE = -760217, ARC_SPEED = -747110 };

enum {
    ON_PEDESTAL, UPPER_RISE, UPPER_ROLL, UPPER_ARC,
    AT_LEDGE, DROP_FROM_LEDGE, REBOUND, AT_FOOT, WINDUP,
    ACCELERATING, LOWER_ROLL, LOWER_ARC, STOPPED
};

static int whole(int32_t n)
{
    return n / Q - (n % Q < 0);
}

static int16_t product(int a, int b)
{
    int n = a * b;
    return (int16_t)(n / 4096 - (n % 4096 < 0));
}

static void set_root(BfmTowerEscapeHead *h)
{
    h->position[1] = (int16_t)whole(h->y);
    h->position[2] = (int16_t)whole(h->z);
    BfmForestTransform yaw, roll;
    bfm_forest_actor_root((unsigned)h->yaw & 4095u, h->position, &yaw);
    if (!h->roll) { h->root = yaw; return; }
    bfm_forest_actor_root((unsigned)h->roll & 4095u, h->position, &roll);
    int sine = roll.rotation[0][2], cosine = roll.rotation[0][0];
    /* The detached head lies on its side. Its rotating local up-axis is
     * horizontal in world space as it travels along the course. */
    h->root = yaw;
    for (unsigned col = 0; col < 3; ++col) {
        h->root.rotation[0][col] = product(cosine, yaw.rotation[0][col]) -
                                   product(sine, yaw.rotation[1][col]);
        h->root.rotation[1][col] = product(sine, yaw.rotation[0][col]) +
                                   product(cosine, yaw.rotation[1][col]);
    }
}

int bfm_tower_escape_head_begin(BfmTowerEscapeHead *h, const BfmForestField *room)
{
    if (!h || !room) return 0;
    const uint8_t *list = bfm_forest_room_bytes(room, UINT32_C(0x801a7be4), 4);
    const uint8_t *actors = list ? bfm_forest_room_bytes(room, u32le(list), 40) : NULL;
    if (!actors || u16le(actors + 26) != 41) return 0;
    *h = (BfmTowerEscapeHead){.visible = 1, .yaw = 3072, .motion = ON_PEDESTAL};
    h->position[0] = s16le(actors + 20);
    h->y = s16le(actors + 22) * Q;
    h->z = s16le(actors + 24) * Q;
    set_root(h);
    return 1;
}

static void set_motion(BfmTowerEscapeHead *h, unsigned motion)
{
    h->motion = motion;
    h->age = 0;
}

static void set_position(BfmTowerEscapeHead *h, int y, int z)
{
    h->position[0] = 0;
    h->y = y * Q;
    h->z = z * Q;
    h->vy = h->vz = h->gravity = 0;
}

static void spin(BfmTowerEscapeHead *h, unsigned amount)
{
    h->yaw = (int16_t)(((unsigned)h->yaw + amount) & 4095u);
}

/* Ordinary head probes use a point 81 units below the mesh origin. A roll
 * follows the supporting face, while an arc sweeps from the preceding point. */
static int advance(BfmTowerEscapeHead *h, const BfmForestField *room, int follow)
{
    int16_t from[3] = {h->position[0], (int16_t)(whole(h->y) + 81), (int16_t)whole(h->z)};
    h->vy += h->gravity;
    h->y += h->vy;
    h->z += h->vz;
    int16_t to[3] = {from[0], (int16_t)(whole(h->y) + 81), (int16_t)whole(h->z)};
    int landed = 0;
    if (!bfm_forest_escape_head_probe(room, from, to, follow, &landed)) return 0;
    h->position[0] = to[0];
    h->y += (to[1] - 81 - whole(h->y)) * Q;
    h->z += (to[2] - whole(h->z)) * Q;
    if (landed) h->vy = 0;
    return landed;
}

static void follow_lane(BfmTowerEscapeHead *h)
{
    int z = whole(h->z);
    if (z >= 6389) h->position[0] = 0;
    else if (z < 5390) h->position[0] = 53;
    else h->position[0] = (int16_t)((6387 - z) * 53 / 998);
}

static unsigned road_zone(int z)
{
    static const int16_t bounce[][2] = {
        {12375,12422}, {11575,11590}, {887,902}, {375,390}, {-2900,-2885},
        {-3889,-3455}, {-1918,-1265}, {-2458,-2193}, {-5009,-4603}
    };
    if (z >= -6116 && z <= -6101) return 2;
    for (unsigned i = 0; i < sizeof bounce / sizeof bounce[0]; ++i)
        if (z >= bounce[i][0] && z <= bounce[i][1]) return 1;
    return 0;
}

static void launch_arc(BfmTowerEscapeHead *h, unsigned motion, int speed, int vy, int gravity)
{
    h->vz = speed;
    h->vy = vy * Q;
    h->gravity = gravity * Q;
    h->pending_zone = 0;
    set_motion(h, motion);
}

static void roll(BfmTowerEscapeHead *h, const BfmForestField *room, int upper,
                 const int16_t player[3])
{
    int in_air = h->motion == LOWER_ARC || h->motion == UPPER_ARC;
    if (!in_air) follow_lane(h);
    if (whole(h->z) < player[2] + 128) h->caught = 1;
    spin(h, upper ? h->spin : 128);
    if (in_air) {
        unsigned zone = road_zone(whole(h->z));
        if (zone) h->pending_zone = zone;
        h->roll = (int16_t)(-1056 - 4 * (int)(h->clock & 15u));
        spin(h, upper ? h->spin : 128);
        if (advance(h, room, 0)) {
            h->roll = -1024;
            h->gravity = 0;
            set_motion(h, upper ? UPPER_ROLL : LOWER_ROLL);
        }
        return;
    }
    if (!upper && ((unsigned)h->yaw & 0xf80u) == 896) {
        launch_arc(h, LOWER_ARC, CRUISE, -32, 8);
        return;
    }
    int grounded = advance(h, room, !upper);
    unsigned zone = road_zone(whole(h->z));
    if (upper) {
        if (grounded) {
            h->vz = CRUISE;
            h->vy = -10 * Q;
            h->gravity = 3 * Q / 16;
            h->spin += 8;
        } else if (zone == 1) launch_arc(h, UPPER_ARC, ARC_SPEED, -16, 3);
    } else if (!grounded) {
        launch_arc(h, LOWER_ARC, h->vz, -32, 4);
    } else if (zone == 1 || h->pending_zone == 1) {
        launch_arc(h, LOWER_ARC, ARC_SPEED, -32, 4);
    } else if (zone == 2 || h->pending_zone == 2) {
        h->exit_ready = 1;
        h->vz = h->vy = h->gravity = 0;
        set_motion(h, STOPPED);
    }
}

int bfm_tower_escape_head_tick(BfmTowerEscapeHead *h, const BfmForestField *room,
                               unsigned phase, unsigned age, const int16_t player[3])
{
    if (!h || !room || !player || !h->visible || phase == BFM_ESCAPE_IDLE) return 0;
    ++h->age;
    ++h->clock;
    unsigned previous = h->phase;
    h->phase = phase;
    if (phase == BFM_ESCAPE_REVEAL && age == 19) {
        set_position(h, -1012, 15512);
        h->roll = -1024;
        set_motion(h, UPPER_RISE);
    } else if (phase == BFM_ESCAPE_DROP && phase != previous) {
        set_position(h, -6950, 7257);
        set_motion(h, AT_LEDGE);
    } else if (phase == BFM_ESCAPE_RECOVER && phase != previous) {
        set_position(h, -2854, 7257);
        h->vy = 734003;
        h->gravity = 4 * Q;
        h->yaw = 0;
        set_motion(h, DROP_FROM_LEDGE);
    } else if (phase == BFM_ESCAPE_RECOVER && age == 98) {
        set_motion(h, WINDUP);
    } else {
        switch (h->motion) {
        case ON_PEDESTAL:
            /* The eyes flash, then the head turns one revolution over sixteen
             * updates as the tower starts breaking beneath the hero. */
            if (phase == BFM_ESCAPE_WAKE && age >= 9 + 65 + 65 + 11) spin(h, 256);
            if (phase == BFM_ESCAPE_BREAK) spin(h, 256);
            if (phase == BFM_ESCAPE_FALL) h->yaw = 3072;
            break;
        case UPPER_RISE:
            if (h->age > 1) {
                h->y -= 6 * Q;
                spin(h, 32);
            }
            if (whole(h->y) < -1264) {
                h->vz = CRUISE;
                h->vy = -8 * Q;
                h->gravity = Q / 2;
                h->spin = 32;
                set_motion(h, UPPER_ROLL);
            }
            break;
        case UPPER_ROLL:
        case UPPER_ARC:
        case LOWER_ROLL:
        case LOWER_ARC:
            roll(h, room, h->motion == UPPER_ROLL || h->motion == UPPER_ARC, player);
            break;
        case DROP_FROM_LEDGE:
        case REBOUND:
            if (advance(h, room, 0)) {
                if (h->motion == DROP_FROM_LEDGE) {
                    h->vy = -32 * Q;
                    h->gravity = 4 * Q;
                    set_motion(h, REBOUND);
                } else set_motion(h, AT_FOOT);
            }
            break;
        case WINDUP:
            if (h->age >= 6 + 8 + 46) {
                h->vz = -2 * Q;
                h->spin = 0;
                set_motion(h, ACCELERATING);
            }
            break;
        case ACCELERATING:
            h->vz -= Q / 4;
            advance(h, room, 1);
            spin(h, h->spin);
            if (h->spin < 128) h->spin += 2;
            if (h->vz < CRUISE) {
                h->vz = CRUISE;
                h->pending_zone = 0;
                set_motion(h, LOWER_ROLL);
            }
            break;
        default: break;
        }
    }
    set_root(h);
    return 1;
}
