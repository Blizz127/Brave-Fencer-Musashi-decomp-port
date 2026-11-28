/* SPDX-License-Identifier: MIT */
#include "village_arrival_motion.h"
#include "field_rest_pose.h"
#include "forest_motion.h"
#include "forest_pose.h"
#include "forest_status.h"
#include "forest_bank_hop.h"
#include "forest_maze_gate.h"
#include "forest_bank_hop_pose.h"
#include "forest_bank_thorns.h"
#include "field_waypoint.h"
#include "village_ground.h"
#include "village_illness_mine_gate.h"
#include "village_mine_props.h"
#include "village_mine_pit.h"
#include "village_mine_lake_fields.h"
#include "village_mine_shaft_gates.h"
#include "village_mine_decks.h"
#include "hells_valley_boulders.h"
#include "valley_bincho.h"
#include "valley_gate_door.h"
#include "village_residents.h"
#include "woods_lily.h"
#include "woods_box_return.h"
#include "woods_pole_climb.h"
#include "woods_pole_pose.h"
#include "woods_rootrick_hero.h"
#include "woods_carry.h"
#include "woods_slope.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { Q = 65536, GRAVITY = 13*Q/8, RUN_BRAKE_SLOT = 9 };
static const uint32_t scene_base = 0x80128158u, main_base = 0x800cedf8u;
static const double angle_unit = 0.0015339807878856412;

typedef struct {
    const uint8_t *data;
    size_t bytes;
    uint32_t base;
} Region;

struct BfmVillageMotion {
    BfmVillagePlayer player;
    Region scene, resident;
    uint32_t room;
    const uint8_t *grid, *cells, *ride_keys, *impact, *settle, *waypoint[2];
    unsigned key_count, key, delay, turn, countdown;
    uint32_t messages[2];
    BfmBedroomMotion *ordinary;
    BfmVillageGround *ground;
    const uint8_t *outgoing, *cleft, *transfer, *brake_keys;
    const uint8_t *bank_doors[2];
    BfmBedroomRoot bank_entrance;
    BfmForestMazeGate maze_gate;
    struct {
        const uint8_t *actor;
        uint8_t destination[36];
    } *bank_returns;
    unsigned brake_count, brake_frame, brake, ingress, ingress_walk;
    int ingress_stop_pending;
    int airborne, landed, reported_stand, box_air_action;
    BfmWoodsLilies *lilies;
    const BfmHellsValleyBoulders *valley_rocks;
    const BfmValleyBincho *valley_bincho;
    const BfmValleyGateDoor *valley_door;
    unsigned valley_tick;
    const BfmVillageMineGate *mine_gate;
    unsigned mine_gate_tick;
    const BfmVillageMineProps *mine_props;
    unsigned mine_props_tick;
    BfmVillageMineWheels *mine_wheels;
    BfmVillageMineShaftGates shaft_gates;
    int shaft_lake_open;
    const BfmVillageMineLakeFields *lake_fields;
    unsigned lake_field_tick;
    BfmShaftBladeData *lake_blade_data;
    BfmShaftJumpBlade lake_blade;
    BfmShaftBladeFrame lake_blade_frame;
    unsigned lake_blade_flags;
    int lake_square;
    BfmFieldPadMode lake_pad;
    int lake_tired, lake_speed_limit;
    int16_t lake_current[3];
    int lake_current_posted;
    BfmMineShaftFan *mine_fan;
    unsigned shaft_tick, box_release_fields;
    int16_t mine_action_foot[3];
    BfmWoodsJumpPose *mountain_jump;
    BfmWoodsPoleLayout poles;
    BfmWoodsPoleGrip grip;
    BfmWoodsPolePose pole_pose;
    BfmWoodsPoleMove pole_move;
    int pole_caught, pole_air_sweep;
    int32_t pole_jump_speed;
    BfmWoodsJumpPose *field_jumps;
    unsigned maze_departure_field;
    unsigned rider;
    struct {
        int16_t bank[3];
        unsigned age, search_radius;
        int bypass;
    } water;
    BfmWoodsSlope slope;
    int sliding;
    struct {
        int active, stride, steering, pressed, held, previous_x, previous_y;
        int hurt_fall;
        int32_t side_q16, vertical_q16;
        int inherited_velocity;
        unsigned neutral, phase_tick;
        enum BfmForestJumpPhase phase;
    } jump;
    int32_t fall_speed;
    uint16_t material, bank_contact_reason;
    const uint8_t *trail_gate, *woods_entrance;
    BfmWoodsBoxRoute woods_route;
    unsigned departure_tick, departure_walk;
    int wait_for_exit_script;
    BfmBedroomRoot crossing;
    int64_t position[3];
    int32_t starting_speed, speed;
    enum { FOOT_QUIET, FOOT_START, FOOT_STOP, FOOT_RUNNING, FOOT_BRAKING } foot;
    int walking, pending, message_due;
    enum { NO_TALK, TALK_APPROACH, TALK_FACE, TALK_HOLD, READ_SIGN, LENO_TURN, LENO_GESTURE } talk;
    unsigned talk_dispatch;
    int16_t talk_x, talk_z, resident_x, resident_z;
};

static uint32_t read32(const uint8_t *p)
{
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int read16(const uint8_t *p)
{
    int n = p[0] | (int)p[1] << 8;
    return n >= 32768 ? n - 65536 : n;
}

static const uint8_t *at(const Region *r, uint32_t address, size_t length)
{
    if (address < r->base) return NULL;
    size_t offset = address - r->base;
    return offset <= r->bytes && length <= r->bytes - offset ? r->data + offset : NULL;
}

static const uint8_t *room_door(const BfmVillageMotion *m, const uint8_t *room, unsigned entrance)
{
    const uint8_t *doors = at(&m->scene,read32(room+0xc8),4*(entrance+1));
    return doors ? at(&m->scene,read32(doors+4*entrance),36) : NULL;
}

static int64_t high(int64_t value)
{
    return value / Q - (value % Q < 0);
}

static void place(BfmVillageMotion *m, const uint8_t *point)
{
    for (int i = 0; i < 3; ++i)
        m->position[i] = (int64_t)read16(point + 2*i)*Q + (m->position[i] & (Q-1));
}

/* The direct rider resource selects both swords and a facial channel, then
 * cycles through full mounted poses until the actor returns to the ground. */
static int load_ride(BfmVillageMotion *m, const uint8_t *room)
{
    const uint8_t *program = at(&m->scene, 0x80189760u, 24);
    if (!program || read32(program) != 4 || !at(&m->resident, read32(program+4), 4) ||
        read32(program+8) != 16 || read32(program+12) != 18 || read32(program+16) != 15)
        return 0;
    unsigned slot = read32(program+20);
    const uint8_t *faces = at(&m->resident, read32(room+0x50), 35*4);
    if (!faces || slot >= 35) return 0;
    const uint8_t *face = at(&m->resident, read32(faces+4*slot), 8);
    if (!face || (read32(face) & ~7u) != 0x40000000u || read32(face+4) != 0) return 0;
    m->player.pose.face_word = read32(face);
    m->player.pose.show_weapon = m->player.pose.show_lumina = 1;
    m->ride_keys = program + 24;
    for (unsigned i = 0; i < 64; ++i) {
        const uint8_t *key = at(&m->scene, 0x80189778u + 4*i, 4);
        if (!key) return 0;
        if (read32(key) == 1) {
            m->key_count = i;
            return i != 0;
        }
        if (!at(&m->scene, read32(key), 256)) return 0;
    }
    return 0;
}

static void ride_pose(BfmVillageMotion *m)
{
    m->player.pose.pose = at(&m->scene, read32(m->ride_keys + 4*m->key), 256);
    m->key = (m->key+1) % m->key_count;
}

static int load_route(BfmVillageMotion *m)
{
    const uint8_t *q = at(&m->scene, 0x8018979cu, 80);
    if (!q || read32(q) != 0x8018023cu || read32(q+4) != 0x8016f6e0u ||
        read32(q+12) != 0x801705f8u || read32(q+20) != 0x801806b4u ||
        read32(q+24) != 0x80170150u || read32(q+32) != 0x8016f8e4u ||
        read32(q+44) != 0x8016ffdcu || read32(q+52) != 0x80171824u ||
        read32(q+56) != 0x8016f8e4u || read32(q+68) != 0x8016ffdcu ||
        read32(q+76) != 0x8016f494u) return 0;
    m->delay = read32(q+8);
    m->turn = read32(q+28);
    m->waypoint[0] = q+36;
    m->waypoint[1] = q+60;
    m->messages[0] = read32(q+48);
    m->messages[1] = read32(q+72);
    return m->delay < 65536 && m->turn < 4096 &&
        (unsigned)read16(q+42) <= 255 && (unsigned)read16(q+66) <= 255 &&
        at(&m->scene, m->messages[0], 4) && at(&m->scene, m->messages[1], 4);
}

/* SC03 marks a cell with no polygon IDs by the halfword pair (65535,0). */
static const uint8_t *cell(const BfmVillageMotion *m, const BfmBedroomRoot *r)
{
    int x = (r->xyz[0] + 32768)/128 - read16(m->grid);
    int z = (r->xyz[2] + 32768)/128 - read16(m->grid+2);
    int width = read16(m->grid+4), depth = read16(m->grid+6);
    return x >= 0 && z >= 0 && x < width && z < depth ? m->cells + 4*(z*width+x) : NULL;
}

static int shadow(const BfmVillageMotion *m, BfmBedroomRoot *r)
{
    const uint8_t *c = cell(m,r);
    if (c && read32(c) == 65535u) {
        r->shadow_visible = 0;
    } else if (!bfm_bedroom_travel_air_shadow(m->scene.data,m->scene.bytes,m->room,r)) return 0;
    if (m->lilies) bfm_woods_lilies_shadow(m->lilies,r);
    if (m->valley_rocks) {
        BfmForestContact surface;
        if (bfm_hells_valley_boulders_support(m->valley_rocks, BFM_VALLEY_ROCK_SHADOW,
                r->xyz, r->xyz, &surface) && (!r->shadow_visible || surface.height < r->shadow_xyz[1])) {
            memcpy(r->shadow_xyz, r->xyz, sizeof r->shadow_xyz);
            memcpy(r->shadow_normal, surface.normal, sizeof r->shadow_normal);
            r->shadow_xyz[1] = (int16_t)surface.height;
            r->shadow_visible = 1;
        }
        int16_t point[3], normal[3];
        if (bfm_hells_valley_hinge_shadow(m->valley_rocks, r->xyz, point, normal) &&
            (!r->shadow_visible || point[1] < r->shadow_xyz[1])) {
            memcpy(r->shadow_xyz, point, sizeof point);
            memcpy(r->shadow_normal, normal, sizeof normal);
            r->shadow_visible = 1;
        }
    }
    return 1;
}

static int publish_root(BfmVillageMotion *m)
{
    BfmBedroomRoot *r = &m->player.root;
    for (int i = 0; i < 3; ++i) {
        int64_t whole = high(m->position[i]);
        if (whole < INT16_MIN || whole > INT16_MAX) return 0;
        r->xyz[i] = (int16_t)whole;
    }
    if (!shadow(m,r)) return 0;
    if (m->player.phase >= BFM_VILLAGE_SETTLE && m->player.phase < BFM_VILLAGE_CONTROL && r->shadow_visible)
        memcpy(r->normal, r->shadow_normal, sizeof r->normal);
    return 1;
}

static int load_field(BfmVillageMotion *m, const uint8_t *room)
{
    uint32_t grid_address = read32(room+0x88);
    m->grid = at(&m->scene, grid_address, 32);
    if (!m->grid) return 0;
    int width = read16(m->grid+4), depth = read16(m->grid+6);
    uint32_t cell_offset = read32(m->grid+8);
    if (width <= 0 || depth <= 0 || width > 512 || depth > 512 ||
        cell_offset < 32 || cell_offset > UINT32_MAX-grid_address) return 0;
    m->cells = at(&m->scene, grid_address+cell_offset, (size_t)width*depth*4);
    if (!m->cells) return 0;
    m->ground = bfm_village_ground_open(m->scene.data,m->scene.bytes,m->room);
    m->ordinary = bfm_bedroom_motion_open(m->scene.data,m->scene.bytes,
                                          m->resident.data,m->resident.bytes,m->room);
    int run_brake = m->room == UINT32_C(0x80186620) || m->room == UINT32_C(0x80190780) ||
                    m->room == UINT32_C(0x8018615c) || m->room == UINT32_C(0x80184870) ||
                    m->room == UINT32_C(0x80181f28) || m->room == UINT32_C(0x80185a50);
    unsigned slot = run_brake ? RUN_BRAKE_SLOT : 5;
    const uint8_t *bank = at(&m->resident,read32(room+0x4c),(slot+1)*4);
    if (!m->ground || !m->ordinary || !bank) return 0;
    uint32_t address = read32(bank+slot*4);
    const uint8_t *brake = at(&m->resident,address,12);
    if (!brake || read32(brake) != 4 || !at(&m->resident,read32(brake+4),4) ||
        read32(brake+8) != 19 || address > UINT32_MAX-12-64*4) return 0;
    m->brake_keys = brake+12;
    for (unsigned i = 0; i < 64; ++i) {
        const uint8_t *key = at(&m->resident,address+12+4*i,4);
        if (!key) return 0;
        if (read32(key) == (run_brake ? 0u : 1u)) {
            m->brake_count = i;
            return i > 1;
        }
        if (!at(&m->resident,read32(key),256)) return 0;
    }
    return 0;
}

BfmVillageMotion *bfm_village_motion_open(const uint8_t *scene, size_t scene_bytes,
    const uint8_t *main, size_t main_bytes, uint32_t room_address, unsigned entrance)
{
    if (!scene || !main || entrance != 5) return NULL;
    BfmVillageMotion *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene,scene_bytes,scene_base};
    m->resident = (Region){main,main_bytes,main_base};
    m->room = room_address;
    const uint8_t *header = at(&m->scene, 0x801854b0u, 4);
    const uint8_t *room = at(&m->scene, room_address, 0xcc);
    if (!header || read32(header) != room_address || !room) goto fail;
    if (!load_field(m,room)) goto fail;
    const uint8_t *entries = at(&m->scene, read32(room+0xc8), (entrance+1)*4);
    const uint8_t *entry = entries ? at(&m->scene, read32(entries+4*entrance), 24) : NULL;
    const uint8_t *velocity = at(&m->scene, 0x80187b0cu, 12);
    m->impact = at(&m->scene, 0x801897fcu, 8);
    m->settle = at(&m->scene, 0x80189830u, 8);
    if (!entry || read32(entry+8) != 0x8018007cu || !m->impact || !m->settle ||
        !velocity || !load_route(m) || !load_ride(m, room)) goto fail;
    uint32_t back = read32(velocity+8);
    if (back < 0u-11u*Q || back == 0) goto fail;
    m->starting_speed = (int32_t)(0u-back);
    m->cleft = at(&m->scene,read32(entries+8),36);
    if (!m->cleft || read32(m->cleft) != 0x02003025u) goto fail;
    m->outgoing = at(&m->scene,read32(entries+4),36);
    if (!m->outgoing || read16(m->outgoing) != 0x1010 || m->outgoing[2] != 1) goto fail;
    place(m,entry+16);
    m->player.root.yaw = (unsigned)read16(entry+22) & 4095;
    m->player.root.busy = 1;
    m->player.root.normal[1] = -4096;
    m->player.phase = BFM_VILLAGE_RIDE;
    m->countdown = 80;
    /* The constructor installs the first pose before the general actor pass. */
    m->key = 1 % m->key_count;
    ride_pose(m);
    if (publish_root(m)) return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

typedef struct {
    BfmVillageGroundResult result;
    BfmForestContact terrain;
    unsigned rider;
} SlopeSupport;

/* The lake current changes the endpoint of the ordinary grounded sweep. A
 * miss leaves the unshifted requested foot and its fractions intact. */
static BfmVillageGroundResult lake_ground_current(BfmVillageMotion *m, int16_t foot[3],
                                                  BfmForestContact *floor)
{
    int16_t end[3], normal[3] = {0, -4096, 0};
    for (unsigned i = 0; i < 3; ++i) end[i] = (int16_t)(foot[i] + m->lake_current[i]);
    uint16_t material;
    unsigned mask = bfm_village_ground_support_normal(m->ground, 0, m->mine_action_foot,
                                                       end, &material, normal);
    m->lake_current_posted = 0;
    if (material == 0x203) m->player.wall_contact = material;
    if (material == 0x21c) m->bank_contact_reason = material;
    if (!mask) return BFM_VILLAGE_GROUND_AIR;
    memcpy(foot, end, sizeof end);
    floor->flags = material;
    floor->height = end[1];
    floor->slope = mask & 0x2000 ? 0x2000 : mask & 0x4000 ? 0x4000 : 0x8000;
    for (unsigned i = 0; i < 3; ++i) floor->normal[i] = normal[i];
    return mask & 0x6000 ? BFM_VILLAGE_GROUND_FOOTING : BFM_VILLAGE_GROUND_AIR;
}

static int lake_current_at_rest(BfmVillageMotion *m)
{
    int16_t foot[3];
    for (unsigned i = 0; i < 3; ++i) foot[i] = (int16_t)high(m->position[i]);
    BfmForestContact surface = {0};
    BfmVillageGroundResult hit = lake_ground_current(m, foot, &surface);
    for (unsigned i = 0; i < 3; ++i)
        m->position[i] = (int64_t)foot[i] * Q + (m->position[i] & (Q-1));
    if (hit == BFM_VILLAGE_GROUND_FOOTING) {
        m->material = surface.flags;
        memcpy(m->player.root.normal, surface.normal, sizeof surface.normal);
    }
    return hit == BFM_VILLAGE_GROUND_FOOTING;
}

static int displace(BfmVillageMotion *m, const uint32_t velocity[3], int require_floor,
                    BfmWoodsMotionContact *contact, SlopeSupport *sliding)
{
    int64_t next[3];
    BfmBedroomRoot candidate = m->player.root;
    for (int i = 0; i < 3; ++i) {
        uint32_t v = velocity[i];
        int64_t signed_v = (int64_t)v - (v > INT32_MAX ? INT64_C(4294967296) : 0);
        next[i] = m->position[i] + signed_v;
        int64_t whole = high(next[i]);
        if (whole < INT16_MIN || whole > INT16_MAX) return require_floor;
        candidate.xyz[i] = (int16_t)whole;
    }
    if (require_floor) {
        BfmForestContact floor = {0};
        if (sliding) {
            /* A slide first meets actors along its proposed stride. Terrain
             * then tests that corrected foot, independently of actor support. */
            if (m->rider)
                sliding->rider = bfm_woods_lilies_support(m->lilies, m->rider,
                                                         m->player.root.xyz, candidate.xyz);
            if (!sliding->rider) {
                int stationary = !memcmp(m->player.root.xyz, candidate.xyz, sizeof candidate.xyz);
                sliding->rider = bfm_woods_lilies_land(m->lilies, m->player.root.xyz,
                                                      candidate.xyz, stationary);
            }
        }
        unsigned hinge_rider = 0;
        BfmForestContact hinge_floor;
        int hinge_hit = 0;
        if (m->valley_rocks) {
            bfm_hells_valley_hinge_body(m->valley_rocks, m->rider, m->player.root.xyz,
                                        candidate.xyz, m->valley_tick, m->airborne);
            for (unsigned leaf = 0; leaf < 2; ++leaf) {
                const BfmForestGuardBox *box;
                const BfmForestTransform *body;
                if (bfm_valley_gate_door_shape(m->valley_door, leaf, &box, &body))
                    bfm_valley_box_block_body(box, body, m->player.root.xyz, candidate.xyz,
                                             m->valley_tick, m->airborne);
            }
            if (m->valley_bincho)
                bfm_valley_bincho_body(m->valley_bincho, m->player.root.xyz, candidate.xyz,
                                      m->valley_tick, m->airborne);
            if (!m->airborne || (int32_t)velocity[1] > 0) {
                int short_probe = !velocity[0] && !velocity[2] && !m->jump.vertical_q16;
                hinge_hit = bfm_hells_valley_hinge_foot(m->valley_rocks, m->rider,
                    short_probe, m->player.root.xyz, candidate.xyz, &hinge_rider, &hinge_floor);
            }
        }
        /* The shaft rotor carries the raw foot after the player publishes the body. */
        const int16_t *previous_foot = m->mine_fan ? m->mine_action_foot : m->player.root.xyz;
        if (m->mine_fan)
            bfm_mine_shaft_fan_body(m->mine_fan, (int)m->rider - 1, previous_foot,
                                    candidate.xyz, m->shaft_tick, m->airborne);
        if (m->mine_props)
            bfm_village_mine_props_body(m->mine_props, m->player.root.xyz, candidate.xyz,
                                        m->mine_props_tick, m->airborne);
        if (m->lake_fields) {
            int face_entry = 0;
            bfm_village_mine_lake_fields_body_contact(m->lake_fields, previous_foot, candidate.xyz,
                                                      m->lake_field_tick, m->airborne, &face_entry);
            if (m->airborne && face_entry) {
                /* The ankle cancels local motion; gravity has its own accumulator. */
                m->speed = m->jump.side_q16 = m->jump.vertical_q16 = 0;
            }
        }
        BfmMineBasketContact deck_contact = BFM_MINE_BASKET_MISS;
        int16_t deck_foot[3];
        int deck_stationary = 0;
        if ((m->mine_wheels || m->mine_fan) && (!m->airborne || (int32_t)velocity[1] > 0)) {
            int rider = (int)m->rider - 1;
            /* Gravity and the preceding basket carry do not set the action's
             * movement bit. Only a moving action sweeps from the last
             * published root. Retention also uses this field's pre-action foot. */
            int moved = velocity[0] || velocity[2] ||
                (velocity[1] && (!m->airborne || (m->jump.inherited_velocity && m->jump.vertical_q16)));
            const int16_t *previous = moved ? previous_foot : candidate.xyz;
            deck_stationary = !memcmp(previous, candidate.xyz, sizeof candidate.xyz);
            if (m->mine_fan)
                deck_contact = bfm_mine_shaft_fan_probe(m->mine_fan, previous, candidate.xyz,
                                                       m->mine_action_foot, &rider, deck_foot);
            else
                deck_contact = bfm_village_mine_decks_probe(m->mine_wheels, previous, candidate.xyz,
                                                            m->mine_action_foot, &rider, deck_foot);
            m->rider = (unsigned)(rider + 1);
            if (deck_contact == BFM_MINE_BASKET_BOARD || deck_contact == BFM_MINE_BASKET_RIDE ||
                deck_contact == BFM_MINE_BASKET_TOUCH)
                memcpy(candidate.xyz, deck_foot, sizeof candidate.xyz);
        }
        if (m->mine_gate) {
            bfm_village_mine_gate_block_body(m->mine_gate, m->player.root.xyz,
                                             candidate.xyz, m->mine_gate_tick, m->airborne);
            /* The initial foot probe can cross a marked surface without placing
             * the actor on it. The following floor and wall sweep owns the step. */
            int16_t probe[3];
            uint16_t marked = 0;
            memcpy(probe, candidate.xyz, sizeof probe);
            bfm_village_ground_support_probe(m->ground, m->airborne ? 1 : 0,
                                            m->player.root.xyz, probe, &marked);
            bfm_forest_bank_latch_material(&m->bank_contact_reason,marked);
        }
        uint16_t mine_contact = 0;
        if (m->woods_route.room >= 0x3044 && m->woods_route.room <= 0x3046) {
            /* A stride can cross the marked passage and its sloped lip together.
             * Capture the marker before the terrain sweep projects the endpoint. */
            int16_t stride[3];
            memcpy(stride, candidate.xyz, sizeof stride);
            bfm_village_ground_support_probe(m->ground, m->airborne != 0, previous_foot,
                                             stride, &mine_contact);
        }
        BfmVillageGroundResult hit;
        if (m->lake_current_posted && !m->airborne) {
            hit = lake_ground_current(m, candidate.xyz, &floor);
        } else if (m->mine_gate || m->maze_gate.actor || m->bank_returns) {
            hit = bfm_village_ground_bank_step(m->ground,m->player.root.xyz,candidate.xyz,
                    m->airborne,&floor,&m->player.wall_contact,&m->bank_contact_reason);
            if (m->field_jumps && m->airborne && hit == BFM_VILLAGE_GROUND_AIR &&
                candidate.xyz[1] > m->player.root.xyz[1] &&
                (candidate.xyz[0] != high(next[0]) || candidate.xyz[2] != high(next[2]))) {
                /* A maze trunk may project the lateral stride away from the
                 * proposed landing. Check the actual descending foot on
                 * that corrected line before declaring that ground is gone. */
                int16_t above[3] = {candidate.xyz[0],m->player.root.xyz[1],candidate.xyz[2]};
                hit = bfm_village_ground_air(m->ground,above,candidate.xyz,&floor);
            }
        } else if ((m->trail_gate || m->valley_rocks || m->woods_route.room == 0x3043 ||
                    m->woods_route.room == 0x3044 || m->woods_route.room == 0x3046) &&
                   !sliding && m->airborne) {
            hit = bfm_village_ground_air_passage(m->ground,m->player.root.xyz,candidate.xyz,
                                                  &floor,&m->player.wall_contact);
        } else if (!sliding && m->airborne) {
            hit = bfm_village_ground_air(m->ground,previous_foot,candidate.xyz,&floor);
        } else {
            hit = bfm_village_ground_wall_step(m->ground,previous_foot,candidate.xyz,&floor,
                                               &m->player.wall_contact);
        }
        if (mine_contact == 0x203) m->player.wall_contact = mine_contact;
        if (m->woods_route.room == 0x3046 && mine_contact == 0x21c) m->bank_contact_reason = mine_contact;
        if (deck_stationary && deck_contact != BFM_MINE_BASKET_MISS)
            memcpy(candidate.xyz, deck_foot, sizeof candidate.xyz);
        if (deck_contact != BFM_MINE_BASKET_MISS && hit == BFM_VILLAGE_GROUND_AIR) {
            floor = (BfmForestContact){.normal={0,-4096,0}, .slope=0x2000};
            hit = BFM_VILLAGE_GROUND_FOOTING;
        }
        if (sliding) {
            sliding->result = hit;
            sliding->terrain = floor;
            if (hit != BFM_VILLAGE_GROUND_BLOCKED) m->rider = sliding->rider;
            if (hit == BFM_VILLAGE_GROUND_AIR && sliding->rider) {
                floor.normal[1] = -4096;
                hit = BFM_VILLAGE_GROUND_FOOTING;
            }
        } else if (m->lilies) {
            /* Gravity advances independently of the regular movement vector.
             * A stationary fall uses the short support ray at the new foot. */
            unsigned pad;
            if (m->airborne && !m->rider && velocity[1] != 0 && velocity[1] <= INT32_MAX) {
                int short_probe = (!velocity[0] && !velocity[2]) ||
                    !memcmp(m->player.root.xyz,candidate.xyz,sizeof candidate.xyz);
                pad = bfm_woods_lilies_land(m->lilies,m->player.root.xyz,candidate.xyz,short_probe);
            } else {
                pad = bfm_woods_lilies_support(m->lilies,m->rider,m->player.root.xyz,candidate.xyz);
            }
            if (pad) {
                floor = (BfmForestContact){.height=candidate.xyz[1], .normal={0,-4096,0}, .slope=0x2000};
                hit = BFM_VILLAGE_GROUND_FOOTING;
            }
            if (hit != BFM_VILLAGE_GROUND_BLOCKED) m->rider = pad;
        }
        if (hinge_hit && hit != BFM_VILLAGE_GROUND_BLOCKED &&
            (hit == BFM_VILLAGE_GROUND_AIR || hinge_floor.height <= candidate.xyz[1])) {
            candidate.xyz[1] = (int16_t)hinge_floor.height;
            floor = hinge_floor;
            hit = BFM_VILLAGE_GROUND_FOOTING;
            m->rider = hinge_rider;
        } else if (m->valley_rocks && hit != BFM_VILLAGE_GROUND_BLOCKED) {
            int16_t foot[3] = {candidate.xyz[0], (int16_t)high(next[1]), candidate.xyz[2]};
            BfmForestContact surface;
            BfmValleyRockProbe kind = m->airborne ? BFM_VALLEY_ROCK_LANDING : BFM_VALLEY_ROCK_PLANTED;
            unsigned rider = bfm_hells_valley_boulders_support(m->valley_rocks, kind,
                m->player.root.xyz, foot, &surface);
            if (rider && (hit == BFM_VILLAGE_GROUND_AIR || surface.height <= candidate.xyz[1])) {
                candidate.xyz[1] = (int16_t)surface.height;
                floor = surface;
                hit = BFM_VILLAGE_GROUND_FOOTING;
                m->rider = rider;
            } else {
                m->rider = 0;
            }
        }
        if (m->poles.count)
            m->pole_air_sweep = m->airborne && hit == BFM_VILLAGE_GROUND_AIR;
        if (hit == BFM_VILLAGE_GROUND_BLOCKED) return 1;
        /* Actor support may have replaced the pit floor underneath the
         * boulder. Report the surface the player actually landed on. */
        if (m->valley_rocks) {
            if (hit == BFM_VILLAGE_GROUND_FOOTING)
                bfm_forest_bank_latch_material(&m->bank_contact_reason, floor.flags);
            bfm_forest_bank_latch_material(&m->bank_contact_reason, m->player.wall_contact);
        }
        if (contact && hit == BFM_VILLAGE_GROUND_FOOTING) {
            contact->slope = floor.slope;
            contact->material = floor.flags;
        }
        /* A steep face can push a falling player sideways without supporting
         * the feet. The ordinary Valley jump uses no contact output. */
        if (hit == BFM_VILLAGE_GROUND_FOOTING && floor.slope == 0x8000 &&
            ((m->mine_wheels || m->mine_fan) ? deck_contact == BFM_MINE_BASKET_MISS :
                (contact || m->woods_route.room == 0x3046 || (m->valley_rocks && m->airborne))))
            hit = BFM_VILLAGE_GROUND_AIR;
        if (contact) {
            contact->landed = m->airborne && hit == BFM_VILLAGE_GROUND_FOOTING;
            contact->lost_support = !m->airborne && hit == BFM_VILLAGE_GROUND_AIR;
        }
        if (hit == BFM_VILLAGE_GROUND_FOOTING) {
            m->landed = m->airborne;
            m->airborne = 0;
            m->material = floor.flags;
            memcpy(m->player.root.normal,floor.normal,sizeof floor.normal);
        } else {
            m->airborne = 1;
            m->material = 0;
            m->player.root.normal[0] = m->player.root.normal[2] = 0;
            m->player.root.normal[1] = -4096;
        }
        for (int i = 0; i < 3; ++i)
            next[i] = (int64_t)candidate.xyz[i]*Q + (next[i] & (Q-1));
    }
    memcpy(m->position,next,sizeof next);
    return 1;
}

static int move(BfmVillageMotion *m, int speed, int vertical, int require_floor)
{
    ForestPlayerState velocity = {0};
    velocity.field_2c.word = -speed;

    if (!bfm_forest_level_velocity(&velocity, 0, (int16_t)m->player.root.yaw, 0)) return 0;
    /* Gravity can pass the horizontal speed bound on a long fall. */
    velocity.velocity[1] = (uint32_t)vertical;
    return displace(m,velocity.velocity,require_floor,NULL,NULL);
}

int bfm_village_motion_view_yaw(const int32_t eye[3], const int32_t target[3])
{
    if (!eye || !target) return 0;
    return bfm_waypoint_heading((int64_t)target[0]-eye[0], (int64_t)target[2]-eye[2]);
}

static int turn(BfmVillageMotion *m)
{
    int delta = (int)((m->turn-m->player.root.yaw+2048) & 4095)-2048;
    /* A positive half-turn keeps its original direction in the greeting. */
    if (m->talk == LENO_TURN && delta == -2048 && m->turn > m->player.root.yaw) delta = 2048;
    int step = delta/4;
    if (!step) step = delta/2;
    if (!step) step = delta;
    m->player.root.yaw = (m->player.root.yaw + (unsigned)step) & 4095;
    return delta == 0;
}

static int walk_toward(BfmVillageMotion *m, int dx, int dz, int power, int camera,
                       BfmBedroomAction *action)
{
    BfmWaypointInput input = bfm_waypoint_input(dx, dz, power, camera);
    m->player.root.yaw = input.yaw;
    /* Keeping WALK selected preserves the walking cycle at partial deflection. */
    if (power || !m->walking) *action = BFM_BEDROOM_WALK;
    if (!m->walking) {
        m->walking = 1;
        return 1;
    }
    return move(m, input.speed_q16, 0, 1);
}

static int waypoint(BfmVillageMotion *m, unsigned leg, int camera, BfmBedroomAction *action)
{
    const uint8_t *point = m->waypoint[leg];
    int dx = read16(point) - m->player.root.xyz[0], dz = read16(point+4) - m->player.root.xyz[2];
    if ((int64_t)dx*dx + (int64_t)dz*dz < 16*16) {
        place(m,point);
        *action = BFM_BEDROOM_STAND;
        m->walking = 0;
        m->message_due = 1;
        return 1;
    }
    return walk_toward(m,dx,dz,read16(point+6),camera,action);
}

static int talking_input(BfmVillageMotion *m, int camera, BfmBedroomAction *action)
{
    /* The resident accepts the input, then dispatches the player action.
     * Both updates precede the first synthesized walking input. */
    if (m->talk_dispatch) {
        --m->talk_dispatch;
        return 1;
    }
    if (m->talk == READ_SIGN) {
        turn(m);
        return 1;
    }
    if (m->talk == TALK_APPROACH) {
        int dx = m->talk_x - m->player.root.xyz[0], dz = m->talk_z - m->player.root.xyz[2];
        if ((int64_t)dx*dx + (int64_t)dz*dz >= 16*16)
            return walk_toward(m,dx,dz,0,camera,action);
        m->position[0] = (int64_t)m->talk_x*Q + (m->position[0] & (Q-1));
        m->position[2] = (int64_t)m->talk_z*Q + (m->position[2] & (Q-1));
        m->walking = 0;
        *action = BFM_BEDROOM_STAND;
        m->turn = (unsigned)bfm_waypoint_heading(m->talk_x-m->resident_x,m->talk_z-m->resident_z);
        m->talk = TALK_FACE;
    } else if (m->talk == TALK_FACE && turn(m)) {
        m->talk = TALK_HOLD;
    }
    return 1;
}

static void mine_begin_fall(BfmVillageMotion *m, int striding, BfmBedroomAction *action)
{
    /* Losing floor support selects the normal falling action without
     * resetting the vertical velocity already changed by the fan. */
    m->airborne = 1;
    m->jump.active = m->jump.hurt_fall = m->jump.inherited_velocity = 1;
    m->jump.phase = BFM_FOREST_JUMP_FALLING;
    m->jump.phase_tick = m->jump.neutral = 0;
    m->jump.steering = m->jump.stride = striding;
    m->jump.side_q16 = m->jump.vertical_q16 = 0;
    if (!striding) m->speed = 0;
    *action = BFM_BEDROOM_FALL;
}

static int mine_stride(const BfmVillageMotion *m)
{
    return m->mine_fan || m->woods_route.room == 0x3046;
}

static int mine_has_stationary_support(BfmVillageMotion *m, unsigned terrain_mode)
{
    if (m->lake_current_posted) return lake_current_at_rest(m);
    int16_t foot[3];
    memcpy(foot, m->mine_action_foot, sizeof foot);
    if (m->mine_fan) {
        int rider = (int)m->rider - 1;
        BfmMineBasketContact deck = bfm_mine_shaft_fan_probe(m->mine_fan, m->mine_action_foot,
            m->mine_action_foot, m->mine_action_foot, &rider, foot);
        m->rider = (unsigned)(rider + 1);
        if (deck != BFM_MINE_BASKET_MISS) {
            for (unsigned axis = 0; axis < 3; ++axis)
                m->position[axis] = (int64_t)foot[axis]*Q + (m->position[axis] & (Q-1));
            return 1;
        }
    }
    int16_t end[3] = {foot[0], (int16_t)(foot[1]+8), foot[2]};
    uint16_t material;
    unsigned mask = bfm_village_ground_support_probe(m->ground, terrain_mode, foot, end, &material);
    return (mask & (0x2000u | 0x4000u)) != 0;
}

static int ordinary_input(BfmVillageMotion *m, int x, int y, int camera,
                          BfmBedroomAction *action, int *standing)
{
    int magnitude = bfm_pad_magnitude(x,y);
    int steer = magnitude >= 16;
    if (magnitude > 127) magnitude = 127;
    /* The corridor's first pad field selects the walking pose. Facing
     * follows the stick when that action integrates its first stride. */
    int lake = m->woods_route.room == 0x3046;
    int selecting_walk = !m->walking && !m->airborne;
    int keep_heading = (m->mine_fan && m->landed) || (lake && (m->landed || selecting_walk)) ||
                       (m->woods_route.room == 0x3044 && selecting_walk && !m->landed);
    if (steer && !keep_heading)
        m->player.root.yaw = (unsigned)(camera+bfm_waypoint_heading(-x,y)) & 4095;
    if (m->landed) {
        m->landed = 0;
        m->walking = steer;
        if (m->jump.active && !steer) {
            m->brake = m->speed >= 11*Q;
            m->brake_frame = 0;
            if (m->brake) m->speed = 4*Q;
        }
        *action = steer ? BFM_BEDROOM_WALK : BFM_BEDROOM_STAND;
        if (lake && steer) {
            int lateral = (int)high(m->jump.side_q16);
            int forward = m->jump.steering ? (int)high(-(int64_t)m->speed) : 0;
            int carried = bfm_pad_magnitude(lateral, forward) * Q;
            if (!mine_has_stationary_support(m, 0))
                mine_begin_fall(m, m->jump.side_q16 || (m->jump.steering && m->speed), action);
            m->speed = carried > m->starting_speed ? carried : m->starting_speed;
        } else if (!m->jump.active || steer) m->speed = ((m->speed+Q-1)/Q)*Q;
        if (steer && m->mine_fan && !mine_has_stationary_support(m, 0))
            mine_begin_fall(m, m->jump.side_q16 || (m->jump.steering && m->speed), action);
        return 1;
    }
    if (!steer && !m->airborne) {
        if (m->walking) {
            *action = BFM_BEDROOM_STAND;
            m->foot = FOOT_STOP;
            m->brake = m->speed == 11*Q;
            m->brake_frame = 0;
            m->walking = 0;
        } else if (m->brake) {
            if (++m->brake_frame < m->brake_count) {
                m->foot = FOOT_BRAKING;
                return move(m,4*Q,0,1);
            }
            m->brake = 0;
        } else {
            if (mine_stride(m) && !mine_has_stationary_support(m, 1))
                mine_begin_fall(m, 0, action);
            if (standing) *standing = !m->airborne;
        }
        return 1;
    }
    if (!m->walking && !m->airborne) {
        if (mine_stride(m) && !mine_has_stationary_support(m, 0)) {
            m->speed = m->brake ? 4*Q : 0;
            mine_begin_fall(m, m->brake, action);
            m->brake = 0;
            return 1;
        }
        m->brake = 0;
        m->walking = 1;
        m->speed = m->lake_tired ? 4*Q : m->starting_speed;
        *action = BFM_BEDROOM_WALK;
        m->foot = FOOT_START;
        return 1;
    }
    m->brake = 0;
    int was_airborne = m->airborne;
    int speed = m->speed+Q/16;
    int limit = m->airborne ? 11*Q+5*Q/8 : 11*Q;
    m->speed = speed < limit ? speed : limit;
    if (!m->airborne && m->speed >= 11*Q) m->foot = FOOT_RUNNING;
    if (steer) speed = speed*magnitude/127;
    if (!m->airborne && m->lake_pad) {
        if (m->lake_tired) {
            speed = m->speed = 4*Q;
        } else if (m->lake_pad == BFM_FIELD_PAD_ANALOG) {
            speed = 11*Q*magnitude/127;
            if (speed > m->lake_speed_limit) speed = m->lake_speed_limit;
            m->speed = speed;
        } else if (m->speed > m->lake_speed_limit) {
            /* Digital projects this stride before capping the retained speed. */
            m->speed = m->lake_speed_limit;
        }
    }
    if (!m->airborne && speed < 9*Q+Q/4) *action = BFM_BEDROOM_WALK;
    if (m->airborne && m->fall_speed < 31*Q) m->fall_speed += GRAVITY;
    if (!move(m,speed,m->airborne ? m->fall_speed : 0,1)) return 0;
    if (m->airborne && !was_airborne) {
        if (mine_stride(m)) mine_begin_fall(m, steer || m->speed != 0, action);
        else *action = BFM_BEDROOM_FALL;
    }
    if (m->landed) *action = BFM_BEDROOM_LAND;
    return 1;
}

static int slope_step(BfmVillageMotion *m, int x, int y, int camera)
{
    int steer = x || y;
    if (steer) m->player.root.yaw = (unsigned)(camera + bfm_waypoint_heading(-x, y)) & 4095;
    int32_t world[3], local[3];
    bfm_woods_slope_move(&m->slope, steer, m->player.root.yaw, world, local);
    const uint32_t delta[3] = {(uint32_t)world[0], (uint32_t)world[1], (uint32_t)world[2]};
    SlopeSupport support = {0};
    if (!displace(m, delta, 1, NULL, &support)) return 0;
    if (support.result == BFM_VILLAGE_GROUND_BLOCKED) return 1;
    BfmWoodsSlopeResult result = bfm_woods_slope_contact(&m->slope,
        support.terrain.slope, support.rider != 0, support.terrain.normal);
    if (result == BFM_WOODS_SLOPE_CONTINUE) return 1;
    m->sliding = 0;
    m->landed = 0;
    if (result == BFM_WOODS_SLOPE_IDLE) {
        m->speed = 0;
        return bfm_bedroom_motion_reenter_stance(m->ordinary, -1);
    }
    m->airborne = 1;
    m->rider = 0;
    m->fall_speed = 0;
    m->speed = -local[2];
    if (m->speed > 11*Q + 5*Q/8) m->speed = 11*Q + 5*Q/8;
    m->jump.inherited_velocity = 1;
    m->jump.side_q16 = local[0];
    m->jump.vertical_q16 = local[1];
    m->jump.active = m->jump.hurt_fall = 1;
    m->jump.stride = m->jump.steering = local[0] || local[2];
    m->jump.phase = BFM_FOREST_JUMP_FALLING;
    return bfm_bedroom_motion_reenter_fall(m->ordinary, m->jump.stride);
}

static int woods_input(BfmVillageMotion *m, int x, int y, int camera,
                       BfmBedroomAction *action, int *standing)
{
    if (m->sliding) {
        if (!m->slope.settling || !m->jump.pressed) return slope_step(m, x, y, camera);
        m->sliding = 0;
    }
    int power = bfm_pad_magnitude(x,y);
    if (power > 127) power = 127;
    int steer = power >= 16;
    if (m->jump.pressed && !m->airborne) {
        int recovering = m->jump.active;
        if (m->mine_wheels || m->mine_fan) m->rider = 0;
        m->jump.active = 1;
        if (m->woods_route.room == 0x3046) {
            m->jump.stride = m->brake || (m->walking && m->speed) ||
                (recovering && (m->jump.side_q16 || (m->jump.steering && m->speed)));
        } else {
            m->jump.stride = m->walking && steer && x == m->jump.previous_x && y == m->jump.previous_y;
        }
        m->jump.hurt_fall = m->jump.inherited_velocity = 0;
        m->jump.side_q16 = m->jump.vertical_q16 = 0;
        m->jump.phase = BFM_FOREST_JUMP_RISING;
        m->jump.phase_tick = m->jump.neutral = 0;
        m->jump.steering = steer;
        if (!m->walking && (!recovering || !m->speed)) m->speed = 9*Q - Q/16;
        m->fall_speed = -18*Q;
        m->landed = m->brake = 0;
        m->airborne = 1;
        return 1;
    }
    if (m->jump.active && m->jump.phase != BFM_FOREST_JUMP_LANDING) {
        if (steer) {
            m->player.root.yaw = (unsigned)(camera+bfm_waypoint_heading(-x,y)) & 4095;
            m->jump.steering = 1;
            m->jump.neutral = 0;
            if (m->speed < 8*Q) m->speed = 9*Q;
            else m->speed += Q/16;
            if (m->speed > 11*Q+5*Q/8) m->speed = 11*Q+5*Q/8;
        } else if (++m->jump.neutral > 3) {
            m->jump.side_q16 = m->jump.vertical_q16 = 0;
            m->jump.steering = 0;
            m->speed = 0;
        }
        int speed = m->jump.steering ? m->speed : 0;
        if (steer) speed = speed*power/127;
        /* The launch animation still receives the supporting actor's final
         * movement before its first airborne integration. */
        m->rider = 0;
        if (m->fall_speed < 31*Q)
            m->fall_speed += m->fall_speed < 0 && !m->jump.held ? 21*Q/8 : GRAVITY;
        if (!m->mountain_jump && !mine_stride(m) && m->fall_speed > 31*Q)
            m->fall_speed = 31*Q;
        if (m->jump.inherited_velocity) {
            ForestPlayerState velocity = {0};
            velocity.field_24.word = m->jump.side_q16;
            velocity.field_2c.word = -speed;
            if (!bfm_forest_level_velocity(&velocity, 0, (int16_t)m->player.root.yaw, 0)) return 0;
            velocity.velocity[1] = (uint32_t)m->fall_speed + (uint32_t)m->jump.vertical_q16;
            if (!displace(m, velocity.velocity, 1, NULL, NULL)) return 0;
        } else if (!move(m,speed,m->fall_speed,1)) return 0;
        ++m->jump.phase_tick;
        if (m->landed) {
            m->jump.phase = BFM_FOREST_JUMP_LANDING;
            m->jump.phase_tick = 0;
            m->brake = !steer && m->speed >= 11*Q;
            m->brake_frame = 0;
            *action = BFM_BEDROOM_LAND;
        } else if (m->jump.phase == BFM_FOREST_JUMP_RISING && m->fall_speed >= 0) {
            m->jump.phase = BFM_FOREST_JUMP_FALLING;
            m->jump.phase_tick = 0;
        }
        return 1;
    }
    if (m->jump.active && (++m->jump.phase_tick >= 10 || steer))
        m->jump.active = 0;
    return ordinary_input(m,x,y,camera,action,m->jump.active ? NULL : standing);
}

static int touches_gate(const int16_t foot[3], const uint8_t *actor)
{
    int dy = foot[1] - read16(actor+2);
    return abs(foot[0] - read16(actor)) <= 256 &&
           abs(foot[2] - read16(actor+4)) <= 256 && dy - 3 <= 256 && dy + 1 >= -256;
}

static int pole_input(BfmVillageMotion *m, int x, int y, int camera,
                      BfmBedroomAction *action, int *standing)
{
    bfm_woods_pole_begin_field(&m->grip);
    m->pole_caught = 0;
    if (m->grip.pole) {
        BfmFieldInput input = {.x=x, .y=y, .cross_pressed=m->jump.pressed};
        int16_t feet[3];
        memcpy(feet,m->player.root.xyz,sizeof feet);
        unsigned yaw = m->player.root.yaw;
        BfmWoodsPoleMove move = bfm_woods_pole_step(&m->poles,&m->grip,input,0,feet,&yaw);
        m->pole_move = move;
        m->player.root.yaw = yaw;
        m->position[1] += ((int32_t)feet[1]-m->player.root.xyz[1])*(int64_t)Q;
        if (move == BFM_POLE_JUMP || move == BFM_POLE_FALL) {
            int jumping = move == BFM_POLE_JUMP;
            int steer = x || y;
            m->jump.active = 1;
            m->jump.stride = m->speed != 0 || m->jump.side_q16 != 0;
            m->jump.steering = steer;
            m->jump.inherited_velocity = 1;
            m->jump.phase = jumping ? BFM_FOREST_JUMP_RISING : BFM_FOREST_JUMP_FALLING;
            m->jump.phase_tick = 0;
            m->fall_speed = jumping ? -18*Q : GRAVITY;
            m->speed = jumping && steer ? m->pole_jump_speed : 0;
            if (jumping && steer)
                m->player.root.yaw = (unsigned)(camera+bfm_waypoint_heading(-x,y)) & 4095;
        }
        return 1;
    }
    /* The player checks the grip sensor only after an unsupported airborne
     * sweep. Merely requesting takeoff does not perform that sweep. */
    m->pole_air_sweep = 0;
    if (!woods_input(m,x,y,camera,action,standing)) return 0;
    if (m->pole_air_sweep) {
        int16_t feet[3];
        for (unsigned i=0; i<3; ++i) feet[i]=(int16_t)high(m->position[i]);
        if (bfm_woods_pole_catch(&m->poles,&m->grip,m->player.root.xyz,feet)) {
            m->pole_move = BFM_POLE_HOLD;
            m->pole_caught = 1;
            /* The mine grip centers the whole coordinates and keeps the
             * incoming stride's sub-unit position on both horizontal axes. */
            int mine = m->woods_route.room == 0x3043;
            if (mine) m->player.events |= BFM_VILLAGE_POLE_CATCH;
            m->position[0]=(int64_t)feet[0]*Q + (mine ? m->position[0] & (Q-1) : 0);
            m->position[2]=(int64_t)feet[2]*Q + (mine ? m->position[2] & (Q-1) : 0);
            m->speed=m->fall_speed=0;
            m->walking=m->brake=m->landed=0;
            memset(&m->jump,0,sizeof m->jump);
        }
    }
    return 1;
}

static const uint8_t *exit_floor(const BfmVillageMotion *m)
{
    if (m->room == UINT32_C(0x80190780)) {
        if (m->mine_gate) {
            if (m->bank_contact_reason == 14) return m->outgoing;
            return bfm_village_mine_gate_passage(m->mine_gate, m->bank_contact_reason);
        }
        /* The inn tests the preceding foot sweep on both faces of its threshold. */
        if ((m->material & 255u) == 14 || (m->player.wall_contact & 255u) == 14)
            return m->outgoing;
        return NULL;
    }
    if ((m->player.wall_contact & 0xffu) == 3) {
        const int16_t *foot = m->player.root.xyz;
        if (m->woods_route.room == 0x3045) {
            const uint8_t *door = bfm_village_mine_shaft_contact(&m->shaft_gates, foot,
                                                                m->player.wall_contact);
            if (door == m->shaft_gates.gate[0].door ||
                (m->shaft_lake_open && door == m->shaft_gates.gate[1].door)) return door;
            return NULL;
        }
        if (m->trail_gate && touches_gate(foot,m->trail_gate)) return m->outgoing;
        const uint8_t *woods_door = bfm_woods_box_passage(&m->woods_route,foot,m->player.wall_contact);
        if (woods_door && (m->woods_route.room != 0x3046 || woods_door == m->woods_route.passage))
            return woods_door;
        const uint8_t *maze = bfm_forest_maze_gate_contact(&m->maze_gate,
                                                          m->player.wall_contact, foot);
        if (maze) return maze;
        if (m->room == UINT32_C(0x801910e4)) {
            for (unsigned i = 0; i < 2; ++i)
                if (touches_gate(foot,m->bank_returns[i].actor))
                    return m->bank_returns[i].destination;
        }
    }
    if (m->room == UINT32_C(0x80192b18)) {
        if (m->material == 0x109) return m->bank_doors[1];
        if (m->material == 0x10a) return m->bank_doors[0];
    }
    if (m->material == 0x106) return m->cleft;
    if (m->room != UINT32_C(0x8018ea38) &&
        m->material == (m->room == UINT32_C(0x80192b18) ? 0x105 : 5)) return m->outgoing;
    return NULL;
}

const uint8_t *bfm_village_motion_castle_exit(const BfmVillageMotion *m)
{
    if (!m || m->room != UINT32_C(0x80192b18) || m->player.phase != BFM_VILLAGE_CONTROL ||
        m->transfer || m->material != 0x107) return NULL;
    const uint8_t *room = at(&m->scene, m->room, 0xcc);
    const uint8_t *door = room ? room_door(m, room, 3) : NULL;
    return door && read32(door) == UINT32_C(0x04003000) ? door : NULL;
}

static const uint8_t *pending_exit(const BfmVillageMotion *m)
{
    return m->player.phase == BFM_VILLAGE_CONTROL && !m->transfer ? exit_floor(m) : NULL;
}

static const uint8_t *passage_actor(const BfmVillageMotion *m, const uint8_t *door)
{
    if (m->woods_route.room == 0x3045) return bfm_village_mine_shaft_actor(&m->shaft_gates, door);
    return bfm_woods_passage_actor(&m->woods_route, door);
}

/* The paired passage installs its virtual walk before this field's clock. */
int bfm_village_motion_woods_departing(const BfmVillageMotion *m)
{
    if (!m) return 0;
    const uint8_t *door = m->transfer ? m->transfer : pending_exit(m);
    return passage_actor(m, door) != NULL;
}

/* Consume the preceding sweep's contact before the current action can replace it. */
static void begin_field(BfmVillageMotion *m)
{
    BfmVillagePlayer *p = &m->player;
    const uint8_t *door = pending_exit(m);
    if (m->mine_gate) m->bank_contact_reason = 0;
    m->foot = FOOT_QUIET;
    m->reported_stand = 0;
    p->events = 0;
    p->wall_contact = 0;
    if (door) {
        m->transfer = door;
        m->crossing = p->root;
        p->destination = (uint16_t)read16(m->transfer);
        p->entrance = m->transfer[2];
        p->events |= BFM_VILLAGE_TRANSFER;
    }
}

static int woods_air_pose(BfmVillageMotion *m)
{
    const BfmWoodsJumpPose *poses = m->mountain_jump ? m->mountain_jump : m->field_jumps;
    if (!poses) poses = bfm_woods_lilies_jump_poses(m->lilies);
    enum BfmForestJumpPhase phase = m->jump.phase;
    unsigned tick = m->jump.phase_tick;
    if (!poses) return 0;
    if (m->jump.hurt_fall && phase == BFM_FOREST_JUMP_FALLING) {
        /* A hit restarts the interrupted gait's entry keys. The last one
         * holds for an extra update before the normal falling loop. */
        const BfmForestJumpPoses *clip = m->jump.stride ? &poses->stride : &poses->standing;
        unsigned entry = clip->count[BFM_FOREST_JUMP_RISING];
        if (tick <= entry) phase = BFM_FOREST_JUMP_RISING;
        else tick = tick-entry+1;
    }
    return bfm_woods_jump_pose_apply(poses,m->jump.stride,phase,tick,
                                    m->resident.data,m->resident.bytes,&m->player.pose);
}

/* A supporting platform or a marked bank lip isn't a rescue point. Probe
 * the room mesh itself and keep the last position it accepts unchanged. */
static void remember_woods_bank(BfmVillageMotion *m)
{
    const int16_t *foot = m->player.root.xyz;
    if (m->airborne || m->sliding || m->rider || foot[1] > INT16_MAX - 8) return;
    int16_t end[3] = {foot[0], (int16_t)(foot[1] + 8), foot[2]};
    BfmForestContact floor;
    if (bfm_village_ground_clear_floor(m->ground,foot,end,1,&floor) &&
        !(floor.flags & 255) && !memcmp(end,foot,sizeof end))
        memcpy(m->water.bank,foot,sizeof end);
}

/* Twinpeak's standing and walking actions remember the resolved foot after
 * a gentle terrain sweep. A probe which corrects even one coordinate is
 * unsuitable as the origin for a later rescue. */
static void remember_mountain_bank(BfmVillageMotion *m)
{
    if (m->grip.pole || m->airborne || m->sliding || m->jump.active) return;
    int16_t foot[3];
    for (unsigned i = 0; i < 3; ++i) foot[i] = (int16_t)high(m->position[i]);
    if (foot[1] > INT16_MAX - 8) return;
    int16_t probe[3] = {foot[0], (int16_t)(foot[1] + 8), foot[2]};
    uint16_t material;
    if (bfm_village_ground_support_probe(m->ground, 1, foot, probe, &material) == 0x2000 &&
        !(material & 255) && !memcmp(foot, probe, sizeof foot))
        memcpy(m->water.bank, foot, sizeof foot);
}

/* Search a ring in world space. The bank can be close to a cliff, so each
 * candidate must have a plain landing and a clear path from the saved foot. */
static int shore_probe(BfmVillageMotion *m, unsigned mode, const int16_t start[3],
                       int16_t end[3], BfmForestContact *floor)
{
    if (!m->mountain_jump)
        return bfm_village_ground_clear_floor(m->ground, start, end, mode != 0, floor);
    uint16_t material;
    if (bfm_village_ground_support_probe(m->ground, mode, start, end, &material) != 0x2000)
        return 0;
    floor->flags = material;
    return 1;
}

static int woods_shore_point(BfmVillageMotion *m, unsigned radius, int16_t out[3])
{
    const int16_t *bank = m->water.bank;
    int top = bank[1] - (int)radius;
    if (top < INT16_MIN || top + 31 > INT16_MAX || bank[1] < INT16_MIN + 4) return 0;
    for (unsigned angle = 0; angle < 4096; angle += 32) {
        int sine = (int)lround(4096*sin(angle*angle_unit));
        int cosine = (int)lround(4096*cos(angle*angle_unit));
        int x = bank[0] + (int)floor((int)radius*sine/4096.0);
        int z = bank[2] + (int)floor((int)radius*cosine/4096.0);
        if (x < INT16_MIN || x > INT16_MAX || z < INT16_MIN || z > INT16_MAX) continue;
        int16_t start[3] = {(int16_t)x, (int16_t)top, (int16_t)z};
        int16_t foot[3] = {(int16_t)x, (int16_t)(top+31), (int16_t)z};
        BfmForestContact floor;
        if (!shore_probe(m,0,start,foot,&floor) ||
            (floor.flags & 255)) continue;
        start[0] = bank[0];
        start[1] = (int16_t)(bank[1]-4);
        start[2] = bank[2];
        if (!shore_probe(m,0,start,foot,&floor) ||
            foot[1] > INT16_MAX-8) continue;
        memcpy(start,foot,sizeof start);
        foot[1] += 8;
        if (!shore_probe(m,0,start,foot,&floor) ||
            foot[1] > INT16_MAX-6) continue;
        memcpy(start,foot,sizeof start);
        foot[1] += 6;
        if (!shore_probe(m,2,start,foot,&floor)) continue;
        memcpy(out,start,sizeof start);
        if (!m->mountain_jump ||
            bfm_village_ground_air(m->ground,start,foot,&floor) == BFM_VILLAGE_GROUND_FOOTING)
            memcpy(m->player.root.normal,floor.normal,sizeof floor.normal);
        return 1;
    }
    return 0;
}

static void enter_woods_water_if_touched(BfmVillageMotion *m)
{
    if (!m->woods_entrance || (m->material & 255) != 2 || m->airborne ||
        m->rider || m->water.bypass) return;
    m->player.events |= BFM_VILLAGE_WATER_HIT;
    m->player.controlled = 0;
    m->player.root.busy = 1;
    m->airborne = m->landed = m->walking = m->brake = m->sliding = 0;
    m->player.wall_contact = 0;
    m->speed = m->fall_speed = 0;
    m->material = 0;
    m->water.age = 1;
    m->water.search_radius = 0;
}

static int step_woods_water(BfmVillageMotion *m)
{
    if (m->water.age <= 17) {
        if (m->water.age == 1) m->player.events |= BFM_VILLAGE_WATER_SPLASH;
        m->position[1] += 4*Q + (int)m->water.age*Q/4;
        m->player.water_hidden = m->water.age == 17;
        ++m->water.age;
        if (!publish_root(m) ||
            !bfm_bedroom_motion_step(m->ordinary,BFM_BEDROOM_NONE,&m->player.pose)) return 0;
        if (!m->jump.active) return 1;
        ++m->jump.phase_tick;
        return woods_air_pose(m);
    }
    if (!m->water.search_radius) {
        int settled = 1;
        for (unsigned i = 0; i < 3; ++i) {
            int here = (int)high(m->position[i]);
            int delta = m->water.bank[i] - here;
            int stride = delta/8;
            if (!stride) stride = (delta > 0) - (delta < 0);
            m->position[i] += (int64_t)stride*Q;
            if (stride != delta) settled = 0;
        }
        if (settled) m->water.search_radius = 4;
        return publish_root(m);
    }
    int16_t landing[3];
    if (m->water.search_radius <= 128) {
        if (!woods_shore_point(m,m->water.search_radius,landing)) {
            m->water.search_radius += 4;
            return 1;
        }
    } else {
        for (unsigned i = 0; i < 3; ++i)
            landing[i] = (int16_t)read16(m->woods_entrance+2*i);
        m->player.root.yaw = (unsigned)read16(m->woods_entrance+6) & 4095;
        m->player.root.normal[0] = m->player.root.normal[2] = 0;
        m->player.root.normal[1] = -4096;
    }
    for (unsigned i = 0; i < 3; ++i)
        m->position[i] = (int64_t)landing[i]*Q + (m->mountain_jump ? 0 : m->position[i] & (Q-1));
    m->water.age = m->water.search_radius = 0;
    memset(&m->jump,0,sizeof m->jump);
    m->player.water_hidden = m->player.root.busy = 0;
    m->player.controlled = 1;
    m->player.events |= BFM_VILLAGE_WATER_RETURN;
    return publish_root(m) &&
           bfm_bedroom_motion_step(m->ordinary,BFM_BEDROOM_STAND,&m->player.pose);
}

static int woods_departure(BfmVillageMotion *m, const uint8_t *gate, int camera, BfmBedroomAction *action)
{
    unsigned tick = m->departure_tick;
    int x = 0, y = 0;
    m->player.controlled = 0;
    if (tick == 1) {
        m->player.root.yaw = (unsigned)read16(gate + 14) & 4095;
    } else if (tick >= 2 && tick < 2 + m->departure_walk) {
        /* The scripted pad follows the outgoing hero in the current view.
         * Keep normal gait, acceleration and collision on every field. */
        int relative = ((int)m->player.root.yaw - camera - 2048) & 4088;
        x = bfm_waypoint_axis(relative, 0);
        y = -bfm_waypoint_axis(relative + 1024, 0);
    } else if (tick == 2 + m->departure_walk) {
        m->speed = 0;
        m->brake = 0;
    }
    if (tick <= 2 + m->departure_walk) ++m->departure_tick;
    if (m->lilies || m->mountain_jump) {
        int standing = 0;
        m->jump.pressed = m->jump.held = 0;
        return woods_input(m,x,y,camera,action,&standing);
    }
    return ordinary_input(m,x,y,camera,action,NULL);
}

static int advance(BfmVillageMotion *m, int stick_x, int stick_y, int camera,
                   struct BfmGameplayRun *run, int idle_allowed)
{
    BfmVillagePlayer *p = &m->player;
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    int standing = 0;
    int valley = m->woods_route.room == 0x302b;
    int switchback = m->woods_route.room == 0x3044;
    int station = m->woods_route.room == 0x3045 || m->woods_route.room == 0x3046;
    int lake_grounded = m->lake_current_posted && !m->airborne;
    int lake_player = m->woods_route.room == 0x3046;
    int lake_air = lake_player && m->airborne && m->jump.active;
    int lake_was_grounded = lake_player && !m->airborne;
    int descending = lake_air && m->fall_speed >= 0;
    int swing = lake_air && run && run->has_player &&
                (m->lake_square || bfm_shaft_jump_blade_active(&m->lake_blade));
    BfmForestTransform prior_model;
    if (swing) {
        for (unsigned i = 0; i < 3; ++i) prior_model.translation[i] = p->root.xyz[i];
        bfm_forest_upright_model(p->root.yaw, &prior_model);
    }
    int gait_input = lake_player && !m->airborne && m->lake_pad && p->controlled &&
                     !m->transfer && !m->ingress && !m->sliding;
    int gait_entry = !m->walking || m->landed;
    unsigned station_stride = lake_player && m->ingress >= 2 && m->ingress <= 2 + m->ingress_walk
        ? m->ingress - 1 : 0;
    int run_brake = valley || m->woods_route.room == 0x3043 || switchback || station ||
                    m->room == UINT32_C(0x80190780);
    int was_braking = m->brake;
    begin_field(m);
    switch (p->phase) {
    case BFM_VILLAGE_RIDE:
        ++p->movement;
        if (m->countdown) {
            --m->countdown;
            m->speed += Q/2;
            if (m->speed > 31*Q) m->speed = 31*Q;
            if (!move(m,m->speed,6*Q,0)) return 0;
        } else {
            m->countdown = 6;
            p->phase = BFM_VILLAGE_DROP;
        }
        ride_pose(m);
        break;
    case BFM_VILLAGE_DROP:
        ++p->movement;
        if (m->countdown) {
            --m->countdown;
            if (!move(m,m->speed,10*Q,0)) return 0;
        } else {
            place(m,m->impact);
            p->phase = BFM_VILLAGE_IMPACT;
            p->events |= BFM_VILLAGE_HIT;
        }
        ride_pose(m);
        break;
    case BFM_VILLAGE_IMPACT:
        /* The riding action suspended ordinary fall. On restoring it the
         * actor coasts for one update, then contacts the terrace. The contact
         * fixes whole Y while retaining its fractional descent and gravity. */
        place(m,m->settle);
        m->fall_speed += GRAVITY;
        if (!move(m,m->speed,m->fall_speed,0)) return 0;
        m->position[1] = (int64_t)read16(m->settle+2)*Q + (m->position[1] & (Q-1));
        p->phase = BFM_VILLAGE_SETTLE;
        m->countdown = m->delay+1;
        action = BFM_BEDROOM_ENTER;
        break;
    case BFM_VILLAGE_SETTLE:
        if (m->countdown) --m->countdown;
        else {
            p->phase = BFM_VILLAGE_WAIT_MUSIC;
            p->events |= BFM_VILLAGE_CAMERA;
        }
        break;
    case BFM_VILLAGE_WAIT_MUSIC:
        if (m->pending) {
            m->pending = 0;
            p->phase = BFM_VILLAGE_TURN;
            turn(m);
        }
        break;
    case BFM_VILLAGE_TURN:
        if (turn(m)) p->phase = BFM_VILLAGE_FIRST_WALK;
        break;
    case BFM_VILLAGE_FIRST_WALK:
    case BFM_VILLAGE_SECOND_WALK: {
        unsigned leg = p->phase == BFM_VILLAGE_SECOND_WALK;
        if (m->message_due) {
            m->message_due = 0;
            p->message = m->messages[leg];
            p->events |= BFM_VILLAGE_OPEN_MESSAGE;
            p->phase = leg ? BFM_VILLAGE_SECOND_MESSAGE : BFM_VILLAGE_FIRST_MESSAGE;
        } else if (!waypoint(m,leg,camera,&action)) return 0;
        break;
    }
    case BFM_VILLAGE_FIRST_MESSAGE:
    case BFM_VILLAGE_SECOND_MESSAGE:
        if (m->pending) {
            m->pending = 0;
            p->message = 0;
            if (p->phase == BFM_VILLAGE_FIRST_MESSAGE) {
                p->events |= BFM_VILLAGE_SHOW_HUD;
                p->phase = BFM_VILLAGE_SECOND_WALK;
            } else {
                p->phase = BFM_VILLAGE_CONTROL;
                p->controlled = 1;
                p->root.busy = 0;
                p->events |= BFM_VILLAGE_RELEASE;
            }
        }
        break;
    case BFM_VILLAGE_CONTROL: {
        if (m->water.age) return step_woods_water(m);
        const uint8_t *gate = passage_actor(m, m->transfer);
        if (m->box_release_fields) {
            if (--m->box_release_fields == 0) {
                p->controlled = 1;
                p->events |= BFM_VILLAGE_SHOW_HUD | BFM_VILLAGE_RELEASE;
            }
        } else if (gate) {
            if (m->wait_for_exit_script) {
                p->controlled = 0;
                if (!ordinary_input(m,0,0,camera,&action,NULL)) return 0;
            } else if (!woods_departure(m,gate,camera,&action)) return 0;
        } else if (m->ingress) {
            unsigned step = m->ingress - (unsigned)station;
            if (!step) {
                /* The arrival station installs the pad on its first field. */
                ++m->ingress;
                break;
            }
            if (step == 1) {
                action = BFM_BEDROOM_WALK;
                if (valley) m->foot = FOOT_START;
            } else if (step <= 1 + m->ingress_walk) {
                /* The room entry synthesizes a direction from the actor's
                 * own heading in the current view, at full walking speed. */
                int facing = (int)p->root.yaw;
                int relative = (facing - camera - 2048) & 4088;
                int px = bfm_waypoint_axis(relative,0), pz = bfm_waypoint_axis(relative+1024,0);
                p->root.yaw = (unsigned)(camera+2048+bfm_waypoint_heading(px,pz)) & 4095;
                int power = bfm_pad_magnitude(px,pz);
                if (!move(m,11*Q*(power > 127 ? 127 : power)/127,0,1)) return 0;
                if (valley) m->foot = FOOT_RUNNING;
            } else if (switchback) {
                /* The switchback walk stops at its last scripted foot. The
                 * remaining entrance fields hold that position until control. */
                if (m->ingress == 2 + m->ingress_walk) action = BFM_BEDROOM_STAND;
                else standing = 1;
            } else {
                if (!m->brake) {
                    action = BFM_BEDROOM_STAND;
                    /* The incoming release reaches the braking animation
                     * with eleven four-unit strides left. */
                    m->brake = 1;
                    m->brake_frame = run_brake ? 0 : m->brake_count - 12;
                }
                if (m->ingress_stop_pending) {
                    /* Selecting the standing brake sets its velocity. The
                     * next actor update applies the first coasting stride. */
                    m->ingress_stop_pending = 0;
                    if (station) m->speed = 4*Q;
                    if (valley) {
                        m->foot = FOOT_STOP;
                        p->events |= BFM_VILLAGE_SHOW_HUD;
                    }
                } else if (!ordinary_input(m,0,0,camera,&action,NULL)) return 0;
                if (!valley) m->foot = FOOT_QUIET;
            }
            unsigned release = station ? 2u : (switchback ? 7u : 6u);
            if (step == release + m->ingress_walk) {
                p->controlled = 1;
                m->ingress = 0;
            } else ++m->ingress;
        } else if (m->talk != NO_TALK) {
            if (!talking_input(m,camera,&action)) return 0;
        } else if (m->lilies) {
            remember_woods_bank(m);
            if (!woods_input(m,stick_x,stick_y,camera,&action,&standing)) return 0;
            enter_woods_water_if_touched(m);
        } else if (m->poles.count) {
            if (!pole_input(m,stick_x,stick_y,camera,&action,&standing)) return 0;
            if (m->mountain_jump && !m->grip.pole) {
                enter_woods_water_if_touched(m);
                if (!m->water.age) remember_mountain_bank(m);
            }
        } else if (m->field_jumps) {
            if (!woods_input(m,stick_x,stick_y,camera,&action,&standing)) return 0;
        } else if (!ordinary_input(m,stick_x,stick_y,camera,&action,&standing)) return 0;
        break;
    }
    }
    if (lake_grounded && m->lake_current_posted) lake_current_at_rest(m);
    m->lake_current_posted = 0;
    if (!publish_root(m)) return 0;
    if (p->phase <= BFM_VILLAGE_IMPACT) return 1;
    int brake_ended = run_brake && was_braking && !m->brake && m->brake_frame == m->brake_count;
    /* The ordinary standing worker keeps advancing the face during the
     * slide and selects its current rest cycle on the terminating body key. */
    int brake_stance = run_brake && was_braking && (m->brake || brake_ended);
    int tired = run_brake && run && bfm_field_tired_stance(run->player.record);
    if (run_brake) bfm_bedroom_motion_stance_condition(m->ordinary,tired);
    BfmBedroomStance stance = (brake_stance || (standing && p->controlled)) && idle_allowed && !m->transfer
        ? BFM_BEDROOM_STANCE_IDLE : BFM_BEDROOM_STANCE_INACTIVE;
    int lake_landed = lake_air && m->landed;
    int lake_liftoff = lake_was_grounded && m->airborne && m->jump.active;
    int gait = gait_input && m->walking && !m->airborne && !m->landed && !m->transfer;
    if (!bfm_bedroom_motion_step_field(m->ordinary, gait || station_stride || lake_landed || lake_liftoff ? BFM_BEDROOM_NONE : action,
                                       run, stance, &p->pose)) return 0;
    if (gait || station_stride) {
        unsigned slot = 6;
        if (station_stride) {
            /* The entrance supplies its own full-strength analog pad. */
            gait_entry = station_stride == 1;
            slot = gait_entry ? 6 : 8;
        } else if (m->lake_tired) slot = 10;
        else if (gait_entry) slot = m->lake_pad == BFM_FIELD_PAD_DIGITAL ? 7 : 6;
        else if (m->speed > 9*Q + Q/4) slot = 8;
        else if (m->speed > 7*Q) slot = 7;
        if (!bfm_bedroom_motion_field_gait(m->ordinary, slot, gait_entry, &p->pose)) return 0;
    }
    m->reported_stand = standing && !m->airborne && !m->grip.pole;
    m->box_air_action = m->airborne && !m->grip.pole && !m->water.age;
    if (lake_liftoff) {
        if (!bfm_bedroom_motion_reenter_fall(m->ordinary, m->jump.stride)) return 0;
        bfm_bedroom_motion_publish_upright(m->ordinary, &p->pose);
        return 1;
    }
    if (lake_air) {
        if (swing && !bfm_shaft_jump_blade_step(&m->lake_blade, m->lake_blade_data,
                m->ordinary, run, &prior_model, m->lake_square, m->lake_blade_flags,
                &p->pose, &m->lake_blade_frame)) return 0;
        if (lake_landed) {
            int moving = m->jump.stride || bfm_shaft_jump_blade_active(&m->lake_blade);
            if (!bfm_bedroom_motion_direct_slot(m->ordinary, moving ? 16 : 14)) return 0;
            bfm_bedroom_motion_publish_upright(m->ordinary, &p->pose);
            bfm_shaft_jump_blade_cancel(&m->lake_blade);
        } else if (descending) {
            bfm_bedroom_motion_descend(m->ordinary);
        }
        return 1;
    }
    if (m->grip.pole)
        return bfm_woods_pole_pose_apply(&m->pole_pose,m->ordinary,m->pole_move,m->pole_caught,1,&p->pose);
    if (m->bank_returns && m->airborne && m->fall_speed >= 0)
        bfm_bedroom_motion_descend(m->ordinary);
    if (run_brake) {
        /* The field advances the old body before the action selects its new
         * program. Selection itself publishes the first mounted key. */
        int begin = m->brake && !m->brake_frame;
        if (begin || (brake_ended && !run)) {
            if (!bfm_bedroom_motion_direct_slot(m->ordinary,begin ? RUN_BRAKE_SLOT : 0))
                return 0;
            if (begin && tired &&
                !bfm_bedroom_motion_external_face(m->ordinary,28,1,&p->pose)) return 0;
            bfm_bedroom_motion_publish_upright(m->ordinary,&p->pose);
        }
    } else if (m->brake) {
        p->pose.pose = at(&m->resident,read32(m->brake_keys+4*m->brake_frame),256);
        p->pose.show_lumina = 0;
    }
    if ((m->lilies || m->mountain_jump || m->field_jumps) && m->jump.active) return woods_air_pose(m);
    return 1;
}

int bfm_village_motion_step_field(BfmVillageMotion *m, int x, int y, int camera_yaw,
                                  struct BfmGameplayRun *run)
{
    if (!m || x < -127 || x > 127 || y < -127 || y > 127) return 0;
    BfmVillageMotion next = *m;
    if (!advance(&next,x,y,camera_yaw & 4095,run,1)) return 0;
    *m = next;
    return 1;
}

int bfm_village_motion_step(BfmVillageMotion *m, int x, int y, int camera_yaw)
{
    return bfm_village_motion_step_field(m,x,y,camera_yaw,NULL);
}

int bfm_village_motion_examine_tick(BfmVillageMotion *m, BfmGameplayRun *run)
{
    if (!m || !m->player.controlled || m->player.phase != BFM_VILLAGE_CONTROL ||
        m->talk != NO_TALK || m->airborne || m->transfer) return 0;
    BfmVillageMotion next = *m;
    begin_field(&next);
    /* An accepted interaction suppresses steering and the running brake.
     * Stationary collision still classifies the current foot on this field. */
    const uint32_t stationary[3] = {0};
    if (next.transfer || !displace(&next,stationary,1,NULL,NULL) || !publish_root(&next)) return 0;
    int tired = run && bfm_field_tired_stance(run->player.record);
    if (!bfm_bedroom_motion_examine_tick(next.ordinary,tired,!bfm_gameplay_story_flag(run,303),
                                         &next.player.pose)) return 0;
    next.walking = next.brake = next.speed = next.fall_speed = 0;
    next.player.root.locomotion_edge = BFM_TRAVEL_NONE;
    *m = next;
    return 1;
}

static int32_t pole_takeoff_speed(const BfmVillageMotion *m, uint32_t address)
{
    const uint8_t *impulse = at(&m->scene, address, 12);
    if (!impulse || read32(impulse) || read32(impulse+4)) return 0;
    uint32_t forward = 0u - read32(impulse+8);
    return forward <= 11*Q ? (int32_t)forward : 0;
}

int bfm_village_motion_box_controls(BfmVillageMotion *m, const MusashiDiscMedia *disc)
{
    if (!m || !disc || m->woods_route.room != 0x3026) return 0;
    if (m->mountain_jump) return 1;
    const uint8_t *room = at(&m->scene,m->room,0x50);
    int32_t speed = pole_takeoff_speed(m, UINT32_C(0x80193380));
    if (!room || !speed) return 0;
    BfmWoodsJumpPose *poses = malloc(sizeof *poses);
    if (!poses) return 0;
    if (!bfm_woods_jump_pose_load(poses,disc,read32(room+0x4c)) ||
        !bfm_woods_pole_load(&m->poles,m->scene.data,m->scene.bytes) ||
        !bfm_woods_pole_pose_load(&m->pole_pose,m->scene.data,m->scene.bytes,
                                  m->resident.data,m->resident.bytes)) {
        free(poses);
        return 0;
    }
    m->mountain_jump = poses;
    m->pole_jump_speed = speed;
    return 1;
}

unsigned bfm_village_motion_box_pole(const BfmVillageMotion *m)
{
    return m ? m->grip.pole : 0;
}

int bfm_village_motion_box_air_action(const BfmVillageMotion *m)
{
    return m && m->box_air_action;
}

int bfm_village_motion_box_reported_stand(const BfmVillageMotion *m)
{
    return m && m->reported_stand;
}

int bfm_village_motion_box_step(BfmVillageMotion *m, int x, int y, int camera_yaw,
                                int cross_pressed, int cross_held, struct BfmGameplayRun *run)
{
    if (!m || !m->mountain_jump || x < -127 || x > 127 || y < -127 || y > 127) return 0;
    BfmVillageMotion next = *m;
    next.jump.pressed = cross_pressed;
    next.jump.held = cross_held;
    if (!advance(&next,x,y,camera_yaw & 4095,run,1)) return 0;
    next.jump.pressed = next.jump.held = 0;
    next.jump.previous_x = x;
    next.jump.previous_y = y;
    *m = next;
    return 1;
}

uint16_t *bfm_village_motion_bank_pending(BfmVillageMotion *m)
{
    return m && (m->bank_returns || m->valley_rocks) ? &m->bank_contact_reason : NULL;
}

static void set_rescued_floor(BfmVillageMotion *m)
{
    const int16_t *foot = m->player.root.xyz;
    int16_t probe[3] = {foot[0], (int16_t)(foot[1] + 8), foot[2]};
    BfmForestContact surface;
    if (bfm_village_ground_step(m->ground,foot,probe,&surface) == BFM_VILLAGE_GROUND_FOOTING) {
        m->material = surface.flags;
        memcpy(m->player.root.normal,surface.normal,sizeof surface.normal);
    } else {
        m->material = 0;
        m->player.root.normal[0] = m->player.root.normal[2] = 0;
        m->player.root.normal[1] = -4096;
    }
}

uint16_t bfm_village_motion_mine_pit_material(const BfmVillageMotion *m)
{
    return m && m->woods_route.room == 0x3046 ? m->bank_contact_reason : 0;
}

int bfm_village_motion_mine_pit(BfmVillageMotion *m, BfmVillageMinePit *pit, BfmGameplayRun *run,
    BfmForestBankThornInput input, int accept_floor, BfmForestBankThornFrame *frame)
{
    if (!m || m->woods_route.room != 0x3046 || !pit || !run || !frame) return 0;
    if (!m->airborne && !m->rider && !input.scene_state && m->player.controlled)
        bfm_village_mine_pit_remember(pit, m->ground, m->mine_action_foot);
    input.ground = m->ground;
    input.position = m->position;
    input.entrance = &m->bank_entrance;
    input.pitch = (int)m->player.pose.pitch;
    input.yaw = (int)m->player.root.yaw;
    uint16_t contact = accept_floor ? m->bank_contact_reason : 0;
    if (!bfm_village_mine_pit_step(pit, run, contact, m->fall_speed, &input, frame)) return 0;
    m->bank_contact_reason = 0;
    if (!frame->movement_locked) return 1;

    m->foot = FOOT_QUIET;
    m->player.events = m->player.wall_contact = 0;
    m->lake_current_posted = 0;
    if (frame->started) {
        m->rider = 0;
        m->speed = m->walking = m->brake = m->landed = 0;
        m->airborne = 1;
    }
    m->fall_speed = pit->recovery.vertical_speed;
    m->player.root.yaw = frame->yaw;
    if (!publish_root(m)) return 0;
    if (frame->hide_body) m->player.root.shadow_visible = 0;
    if (frame->recovered) {
        memset(&m->jump, 0, sizeof m->jump);
        m->airborne = 0;
        set_rescued_floor(m);
        if (!bfm_bedroom_motion_reenter_stance(m->ordinary, -1)) return 0;
        bfm_bedroom_motion_publish_upright(m->ordinary, &m->player.pose);
        return bfm_bedroom_motion_external_face(m->ordinary, 17, 1, &m->player.pose);
    }
    /* The pit replaces the movement action while the interrupted body keeps
     * advancing through its mounted animation, including the hidden fields. */
    if (!bfm_bedroom_motion_step(m->ordinary, BFM_BEDROOM_NONE, &m->player.pose)) return 0;
    if (!m->jump.active) return 1;
    ++m->jump.phase_tick;
    return woods_air_pose(m);
}

int bfm_village_motion_mine_release_rider(BfmVillageMotion *m)
{
    if (!m || m->woods_route.room != 0x3046) return 0;
    m->rider = 0;
    if (!publish_root(m)) return 0;
    m->player.root.shadow_visible = 0;
    return 1;
}

int bfm_village_motion_bank_thorn(BfmVillageMotion *m, BfmForestBankThornHurt *hurt,
                                  BfmGameplayRun *run, BfmForestBankThornInput input,
                                  BfmForestBankThornFrame *frame)
{
    if (!bfm_village_motion_bank_pending(m) || !hurt || !run || !frame) return 0;
    if (!m->airborne && !m->rider && !input.scene_state && m->player.controlled)
        bfm_forest_bank_thorn_remember(hurt,m->ground,m->player.root.xyz);
    input.ground = m->ground;
    input.position = m->position;
    input.entrance = &m->bank_entrance;
    input.pitch = (int)m->player.pose.pitch;
    input.yaw = (int)m->player.root.yaw;
    if (!bfm_forest_bank_thorn_hurt_step(hurt,run,&input,frame)) return 0;
    if (!frame->movement_locked) return 1;
    m->foot = FOOT_QUIET;
    m->player.events = m->player.wall_contact = 0;
    if (frame->started) {
        memset(&m->jump, 0, sizeof m->jump);
        m->rider = 0;
        m->speed = m->walking = m->brake = m->landed = m->fall_speed = 0;
        m->airborne = 1;
        if (!bfm_bedroom_motion_direct(m->ordinary,frame->body_program)) return 0;
    }
    m->player.root.yaw = frame->yaw;
    if (!publish_root(m)) return 0;
    if (frame->recovered) {
        set_rescued_floor(m);
        m->airborne = 0;
        m->bank_contact_reason = 0;
        if (!bfm_bedroom_motion_reenter_stance(m->ordinary,-1)) return 0;
    }
    if (!bfm_bedroom_motion_step(m->ordinary,BFM_BEDROOM_NONE,&m->player.pose)) return 0;
    return !frame->recovered ||
        bfm_bedroom_motion_external_face(m->ordinary,17,1,&m->player.pose);
}

int bfm_village_motion_bank_hop(BfmVillageMotion *m, BfmForestBankHop *hop,
                                BfmForestBankHopPose *poses, BfmForestStatus *player,
                                BfmFieldInput field, int camera_yaw, uint32_t actor_flags,
                                int *takeoff)
{
    if (!m || !hop || !player || !takeoff || (!m->bank_returns && !m->field_jumps)) return -1;
    *takeoff = 0;
    if (!m->player.controlled || m->ingress || m->transfer) return 0;
    if (player->record[0x49] != 12) {
        if (hop->active) bfm_forest_bank_hop_pose_reset(poses);
        (void)bfm_forest_bank_hop_step(hop,player,0,0,camera_yaw,m->player.root.yaw,0);
        return 0;
    }
    if (!poses) return -1;
    BfmVillageMotion next = *m;
    BfmForestBankHop ability = *hop;
    BfmForestStatus status = *player;
    begin_field(&next);
    if (!next.transfer) {
        BfmBankHopFrame frame = bfm_forest_bank_hop_step(&ability, &status, field.x,
            field.y, camera_yaw, next.player.root.yaw, field.circle_pressed);
        *takeoff = frame.started;
        if (frame.started) {
            ability.launch_y = next.player.root.xyz[1];
            next.walking = next.brake = next.speed = next.landed = 0;
        }
        if (frame.action == BFM_BANK_HOP_CANCEL) {
            /* A cancellation releases the current arc to ordinary gravity.
             * A fresh launch or rebound still has a floor beneath its foot. */
            int16_t feet[3];
            memcpy(feet,next.player.root.xyz,sizeof feet);
            uint16_t material = 0;
            unsigned support = bfm_village_ground_support_probe(next.ground,0,
                                     next.player.root.xyz,feet,&material);
            bfm_forest_bank_latch_material(&next.bank_contact_reason,material);
            next.airborne = !support || support == 0x8000;
            next.fall_speed = next.airborne ? ability.vertical_q16 : 0;
            if (next.field_jumps && next.airborne) {
                /* Release into the ordinary controllable falling action. It
                 * preserves the stride and lets neutral input arrest drift. */
                next.jump.active = next.jump.hurt_fall = 1;
                next.jump.stride = next.jump.steering = next.speed != 0;
                next.jump.phase = BFM_FOREST_JUMP_FALLING;
                next.jump.phase_tick = next.jump.neutral = 0;
                next.jump.inherited_velocity = 0;
            }
            if (!bfm_forest_bank_hop_pose_step(poses,frame.started,actor_flags,next.ordinary,0,
                                               &next.player.pose) ||
                !bfm_bedroom_motion_leave_hop(next.ordinary,support,next.speed != 0,
                                              &next.player.pose)) return -1;
            bfm_forest_bank_hop_pose_reset(poses);
            next.landed = 0;
        } else {
            next.airborne = 1;
            next.player.root.yaw = frame.yaw;
            next.speed = frame.speed_q16;
            if (!move(&next,frame.speed_q16,frame.vertical_q16,1)) return -1;
            if (next.landed) {
                int16_t foot[3];
                for (int i = 0; i < 3; ++i) foot[i] = (int16_t)high(next.position[i]);
                if (foot[1] > INT16_MAX-8) return -1;
                foot[1] += 8;
                uint16_t material = 0;
                bfm_village_ground_support_probe(next.ground,1,next.player.root.xyz,foot,&material);
                bfm_forest_bank_latch_material(&next.bank_contact_reason,material);
                for (int i = 0; i < 3; ++i)
                    next.position[i] = (int64_t)foot[i]*Q + (next.position[i] & (Q-1));
                ability.launch_y = foot[1];
                bfm_forest_bank_hop_rebound(&ability);
                *takeoff = 1;
                next.airborne = 1;
                next.landed = 0;
            }
            next.fall_speed = ability.vertical_q16;
            if (!bfm_forest_bank_hop_pose_step(poses,*takeoff,actor_flags,next.ordinary,0,&next.player.pose))
                return -1;
        }
        if (!publish_root(&next)) return -1;
    }
    *hop = ability;
    *player = status;
    *m = next;
    return 1;
}

static int advance_platforms(BfmVillageMotion *m, int carry_rider)
{
    int16_t carry[3];
    bfm_woods_lilies_tick(m->lilies,m->rider,m->player.root.xyz,carry);
    if (!m->rider || !carry_rider) return 1;
    /* The first airborne stride owns X/Z but still starts at the pad's
     * current height. Guard recoil uses this same moving-platform pass. */
    if (m->airborne) carry[0] = carry[2] = 0;
    for (unsigned i = 0; i < 3; ++i) m->position[i] += (int64_t)carry[i]*Q;
    return publish_root(m);
}

int bfm_village_motion_step_woods_field(BfmVillageMotion *m, int x, int y, int camera_yaw,
    int cross_pressed, int cross_held, int actors_running, BfmWoodsLilies *lilies,
    struct BfmGameplayRun *run, int idle_allowed)
{
    if (!m || !lilies || x < -127 || x > 127 || y < -127 || y > 127 ||
        m->room != UINT32_C(0x8018ea38)) return 0;
    BfmVillageMotion next = *m;
    next.lilies = lilies;
    next.jump.pressed = cross_pressed;
    next.jump.held = cross_held;
    if (actors_running && !advance_platforms(&next,1)) return 0;
    if (!advance(&next,x,y,camera_yaw & 4095,run,actors_running && idle_allowed)) return 0;
    next.jump.previous_x = x;
    next.jump.previous_y = y;
    *m = next;
    return 1;
}

void bfm_village_motion_woods_exit_script(BfmVillageMotion *m, int enabled)
{
    if (!m || m->woods_route.room < 0x3043 || m->woods_route.room > 0x3046) return;
    if (enabled && m->wait_for_exit_script) m->departure_tick = 0;
    m->wait_for_exit_script = !enabled;
}

int bfm_village_motion_woods_departure_step(BfmVillageMotion *m, int camera_yaw,
                                            struct BfmGameplayRun *run)
{
    if (!m || !passage_actor(m, m->transfer)) return 0;
    if (m->lilies)
        return bfm_village_motion_step_woods_field(m,0,0,camera_yaw,0,0,1,m->lilies,run,0);
    return bfm_village_motion_step_field(m,0,0,camera_yaw,run);
}

int bfm_village_motion_step_woods(BfmVillageMotion *m, int x, int y, int camera_yaw,
                                 int cross_pressed, int cross_held, int actors_running,
                                 BfmWoodsLilies *lilies)
{
    return bfm_village_motion_step_woods_field(m,x,y,camera_yaw,cross_pressed,cross_held,
                                               actors_running,lilies,NULL,0);
}

int bfm_village_motion_leno_greeting(BfmVillageMotion *m, BfmWoodsLilies *lilies,
    int actors_running, const int16_t target[3], int speaking)
{
    if (!m || !target || !lilies || m->room != UINT32_C(0x8018ea38) ||
        (m->talk != NO_TALK && m->talk != LENO_TURN && m->talk != LENO_GESTURE)) return 0;
    BfmVillageMotion next = *m;
    next.lilies = lilies;
    if (actors_running && !advance_platforms(&next, 0)) return 0;
    begin_field(&next);

    BfmBedroomAction action = BFM_BEDROOM_NONE;
    if (next.talk == NO_TALK) {
        if (!next.player.controlled) return 0;
        /* The script interrupts locomotion at the earned meeting point. */
        next.turn = (unsigned)bfm_waypoint_heading(next.player.root.xyz[0] - target[0],
                                                   next.player.root.xyz[2] - target[2]);
        next.talk_dispatch = 2;
        next.talk = LENO_TURN;
        next.player.controlled = 0;
        next.player.root.busy = 1;
        next.walking = next.brake = next.landed = next.speed = next.fall_speed = 0;
        memset(&next.jump, 0, sizeof next.jump);
        action = BFM_BEDROOM_ENTER;
    }
    if (next.talk_dispatch) --next.talk_dispatch;
    else if (next.talk == LENO_TURN) turn(&next);
    if (speaking && next.talk == LENO_TURN) {
        /* The greeting uses the resident's slot 33 collecting gesture. */
        next.talk = LENO_GESTURE;
        action = BFM_BEDROOM_COLLECT;
    }
    if (!publish_root(&next) ||
        !bfm_bedroom_motion_step(next.ordinary, action, &next.player.pose)) return 0;
    *m = next;
    return 1;
}

int bfm_village_motion_end_leno_greeting(BfmVillageMotion *m)
{
    if (!m || (m->talk != LENO_TURN && m->talk != LENO_GESTURE)) return 0;
    m->talk = NO_TALK;
    m->player.controlled = 1;
    m->player.root.busy = 0;
    return 1;
}

static int carry_pose(BfmVillageMotion *m, const BfmWoodsCarryFrame *frame, int bracing)
{
    BfmBedroomPose *body = &m->player.pose;
    if (frame->pose < 16) return 0;
    const uint8_t *mounted = at(&m->resident,frame->pose-16,256);
    if (!mounted || !bfm_bedroom_motion_external_face(m->ordinary,frame->face_slot,bracing,body)) return 0;
    body->pose = mounted;
    body->show_lumina = 0;
    if (!bracing) body->show_weapon = 0;
    return 1;
}

int bfm_village_motion_brace(BfmVillageMotion *m, const BfmWoodsCarryFrame *frame)
{
    return m && frame && (!frame->pose || carry_pose(m,frame,1));
}

int bfm_village_motion_carry(BfmVillageMotion *m, int x, int y, int camera, int actors_running,
                             BfmWoodsLilies *lilies, const BfmWoodsCarryFrame *frame)
{
    if (!m || !frame || !lilies || !m->player.controlled || m->airborne ||
        m->room != UINT32_C(0x8018ea38) || x < -127 || x > 127 || y < -127 || y > 127) return 0;
    BfmVillageMotion next = *m;
    next.lilies = lilies;
    if (actors_running && !advance_platforms(&next,1)) return 0;
    begin_field(&next);
    if (frame->event == BFM_CARRY_LIFT) {
        memset(&next.jump,0,sizeof next.jump);
        next.walking = next.landed = next.brake = next.speed = next.fall_speed = 0;
    }
    if (frame->turn)
        next.player.root.yaw = (unsigned)(camera+bfm_waypoint_heading(-x,y)) & 4095;
    if (!move(&next,frame->speed_q16,0,1) || !publish_root(&next)) return 0;
    if (next.airborne) {
        next.speed = frame->speed_q16;
        next.fall_speed = 0;
        if (!bfm_bedroom_motion_step(next.ordinary,BFM_BEDROOM_FALL,&next.player.pose)) return 0;
    } else if (frame->finished) {
        if (!bfm_bedroom_motion_step(next.ordinary,BFM_BEDROOM_REST,&next.player.pose)) return 0;
    } else if (!carry_pose(&next,frame,0)) return 0;
    *m = next;
    return 1;
}

/* Sample the support top at fixed X/Z on either side of the platform tick.
 * A rising pad can meet stationary airborne feet while the hurt owns XYZ. */
static unsigned lily_top(const BfmWoodsLilies *lilies, const int16_t feet[3], int16_t *height)
{
    if (!lilies) {
        *height = 0;
        return 0;
    }
    const int16_t above[3] = {feet[0], INT16_MIN, feet[2]};
    int16_t below[3] = {feet[0], INT16_MAX, feet[2]};
    unsigned pad = bfm_woods_lilies_support(lilies,0,above,below);
    *height = below[1];
    return pad;
}

int bfm_village_motion_woods_reaction(BfmVillageMotion *m, BfmWoodsLilies *lilies,
    BfmWoodsReactionMotion kind, int32_t dx_q16, int32_t dz_q16, BfmWoodsMotionContact *contact)
{
    if (!m || !contact || !m->player.controlled ||
        (m->room != (lilies ? UINT32_C(0x8018ea38) : UINT32_C(0x801910e4)) &&
         !(m->woods_route.room == 0x3044 && !lilies) &&
         !(m->woods_route.room == 0x3046 && !lilies) &&
         !(m->mountain_jump && !lilies && kind != BFM_WOODS_GUARD_RECOIL &&
           m->woods_route.room == 0x3026)) ||
        (unsigned)kind > BFM_WOODS_GUARD_RECOIL ||
        (kind != BFM_WOODS_GUARD_RECOIL && (dx_q16 || dz_q16))) return 0;
    BfmVillageMotion next = *m;
    next.lilies = lilies;
    if (kind == BFM_WOODS_HURT_START) {
        /* A wall grip is action 20. Class-one hurt starts a footing check
         * immediately even though Musashi was suspended above the ground. */
        if (next.mountain_jump) next.airborne = next.box_air_action;
        next.sliding = 0;
        if (!next.jump.active) {
            next.jump.steering = next.walking;
            next.jump.stride = next.walking;
        }
        if (!next.jump.steering) next.speed = 0;
        next.fall_speed = 0;
        next.brake = next.landed = next.walking = next.jump.active = 0;
    }
    const int16_t *feet = m->player.root.xyz;
    int16_t before_y = 0;
    unsigned before_pad = kind == BFM_WOODS_GUARD_RECOIL ? 0 : lily_top(lilies,feet,&before_y);
    if (lilies && !advance_platforms(&next,kind == BFM_WOODS_GUARD_RECOIL)) return 0;
    begin_field(&next);
    BfmWoodsMotionContact hit = {0};
    if (kind == BFM_WOODS_GUARD_RECOIL) {
        next.speed = next.fall_speed = 0;
        const uint32_t delta[3] = {(uint32_t)dx_q16,0,(uint32_t)dz_q16};
        if (!displace(&next,delta,1,&hit,NULL)) return 0;
    } else {
        int16_t after_y;
        unsigned after_pad = lily_top(lilies,feet,&after_y);
        if (next.airborne && before_pad && after_pad == before_pad && before_y >= feet[1] &&
            after_y <= feet[1] && after_y < before_y) {
            hit.landed = 1;
            hit.slope = 0x2000;
            next.rider = after_pad;
            next.airborne = 0;
            next.material = 0;
            next.player.root.normal[0] = next.player.root.normal[2] = 0;
            next.player.root.normal[1] = -4096;
        } else if (!next.airborne) {
            /* Recheck support without accepting any new walking movement.
             * A pad that has moved below the frozen feet no longer carries them. */
            if (lilies && next.rider && (after_pad != next.rider || after_y > feet[1])) next.rider = 0;
            const uint32_t still[3] = {0};
            if (!displace(&next,still,1,&hit,NULL)) return 0;
            memcpy(next.position,m->position,sizeof next.position);
        }
    }
    if (hit.material) enter_woods_water_if_touched(&next);
    if (!publish_root(&next)) return 0;
    *m = next;
    *contact = hit;
    return 1;
}

int bfm_village_motion_field_rest(BfmVillageMotion *m, BfmFieldRestPose phase, unsigned tick)
{
    if (!m || !m->player.controlled || m->talk != NO_TALK) return 0;
    int result = bfm_bedroom_motion_field_rest(m->ordinary, phase, tick, &m->player.pose);
    if (result) {
        begin_field(m);
        m->walking = 0;
        m->speed = 0;
        m->player.root.locomotion_edge = BFM_TRAVEL_NONE;
    }
    return result;
}

int bfm_village_motion_ability_pose(BfmVillageMotion *m, unsigned face_slot, int finished)
{
    return m && bfm_bedroom_motion_ability_pose(m->ordinary, face_slot, finished, &m->player.pose);
}

int bfm_village_motion_combat_face(BfmVillageMotion *m, unsigned slot)
{
    return m && bfm_bedroom_motion_external_face(m->ordinary, slot, 0, &m->player.pose);
}

int bfm_village_motion_woods_cut(BfmVillageMotion *m, BfmWoodsLilies *lilies,
                                unsigned cancel_countdown, int finishing, int *supported)
{
    if (!m || !lilies || !supported || !m->player.controlled || m->room != UINT32_C(0x8018ea38))
        return 0;
    m->lilies = lilies;
    if (cancel_countdown) m->slope.remaining = cancel_countdown;
    if (!advance_platforms(m, 1)) return 0;
    begin_field(m);
    *supported = 1;
    if (finishing) return publish_root(m) && bfm_village_motion_woods_recover(m, 0);

    /* The action retains the recoil's planted feet; ordinary directional
     * input and jump are not sampled during the cut. */
    const uint32_t still[3] = {0};
    BfmWoodsMotionContact contact = {0};
    if (!displace(m, still, 1, &contact, NULL) || !publish_root(m)) return 0;
    if (contact.lost_support || m->airborne) {
        *supported = 0;
        if (m->speed > 0xba000) m->speed = 0xba000;
        m->jump.stride = m->speed != 0;
        if (!bfm_bedroom_motion_reenter_fall(m->ordinary, m->jump.stride) ||
            !bfm_village_motion_woods_recover(m, 1)) return 0;
        m->jump.inherited_velocity = 1;
        return 1;
    }
    if (contact.slope & 0x4000) {
        m->sliding = 1;
        m->landed = m->brake = m->walking = 0;
        bfm_woods_slope_begin(&m->slope, m->player.root.normal, m->slope.remaining);
        *supported = 0;
        return bfm_bedroom_motion_reenter_slide(m->ordinary) &&
               bfm_bedroom_motion_step(m->ordinary, BFM_BEDROOM_NONE, &m->player.pose);
    }
    return 1;
}

int bfm_village_motion_woods_recover(BfmVillageMotion *m, int resume_fall)
{
    if (!m || !m->player.controlled ||
        m->room != (m->lilies ? UINT32_C(0x8018ea38) : UINT32_C(0x801910e4))) return 0;
    BfmVillageMotion next = *m;
    int stride = next.jump.stride;
    memset(&next.jump,0,sizeof next.jump);
    next.walking = next.brake = next.landed = 0;
    next.airborne = !!resume_fall;
    next.fall_speed = 0;
    if (next.airborne && next.lilies) {
        next.rider = 0;
        next.jump.active = next.jump.hurt_fall = 1;
        next.jump.phase = BFM_FOREST_JUMP_FALLING;
        next.jump.stride = stride;
        next.jump.steering = next.speed != 0;
        if (!woods_air_pose(&next)) return 0;
    } else if (next.airborne) {
        if (!bfm_bedroom_motion_step(next.ordinary,BFM_BEDROOM_FALL,&next.player.pose)) return 0;
    } else {
        next.speed = 0;
        if (!bfm_bedroom_motion_step(next.ordinary,BFM_BEDROOM_REST,&next.player.pose)) return 0;
    }
    *m = next;
    return 1;
}

void bfm_village_motion_woods_condition(BfmVillageMotion *m, int tired)
{
    if (m) bfm_bedroom_motion_stance_condition(m->ordinary, tired);
}

static int resolve_woods_footing(BfmVillageMotion *m, int tired, int blade_held)
{
    if (!m || !m->player.controlled || !m->ground) return 0;
    /* A Valley pit completes its safe-ground placement before the next
     * field can consume a revival item. Never discard a moving-rock rider
     * by treating its feet as resting on the terrain underneath. */
    int valley_return = m->valley_rocks && !m->airborne && !m->rider;
    if (m->room != UINT32_C(0x801910e4) &&
        !valley_return && m->woods_route.room != 0x3044 && m->woods_route.room != 0x3046 &&
        !(m->woods_route.room == 0x3026 && m->mountain_jump) &&
        (m->room != UINT32_C(0x8018ea38) || !m->lilies)) return 0;
    BfmVillageMotion next = *m;
    int16_t feet[3];
    memcpy(feet, next.player.root.xyz, sizeof feet);
    unsigned rider = 0;
    if (next.lilies) {
        if (next.rider)
            rider = bfm_woods_lilies_support(next.lilies, next.rider, next.player.root.xyz, feet);
        if (!rider) rider = bfm_woods_lilies_land(next.lilies, next.player.root.xyz, feet, 1);
    } else if (next.mine_wheels) {
        int deck = (int)next.rider - 1;
        bfm_village_mine_decks_probe(next.mine_wheels, next.mine_action_foot, next.mine_action_foot,
                                     next.mine_action_foot, &deck, feet);
        rider = (unsigned)(deck + 1);
    }
    next.rider = rider;
    if (rider) {
        for (unsigned i = 0; i < 3; ++i)
            next.position[i] = (int64_t)feet[i]*Q + (next.position[i] & (Q-1));
        if (!publish_root(&next)) return 0;
    }
    /* The standing terrain test extends the usual zero-length floor sweep
     * by eight units downward. Its projected endpoint is used only to
     * classify support; a stationary terrain hit does not snap the feet. */
    if (feet[1] > INT16_MAX - 8) return 0;
    int16_t end[3] = {feet[0], feet[1]+8, feet[2]};
    BfmForestContact floor;
    BfmVillageGroundResult hit;
    if (next.lake_current_posted) {
        memcpy(end, feet, sizeof end);
        hit = lake_ground_current(&next, end, &floor);
        for (unsigned i = 0; i < 3; ++i)
            next.position[i] = (int64_t)end[i] * Q + (next.position[i] & (Q-1));
    } else {
        hit = bfm_village_ground_step(next.ground, feet, end, &floor);
    }
    if (valley_return && (hit != BFM_VILLAGE_GROUND_FOOTING || floor.slope != 0x2000 ||
                         (floor.flags & 255) || end[1] != feet[1])) return 0;
    unsigned support = rider ? 1u : 0u;
    if (hit == BFM_VILLAGE_GROUND_FOOTING) {
        support |= floor.slope;
        next.material = floor.flags;
        memcpy(next.player.root.normal, floor.normal, sizeof floor.normal);
    } else {
        next.material = 0;
        next.player.root.normal[0] = next.player.root.normal[2] = 0;
        next.player.root.normal[1] = -4096;
    }
    next.landed = next.walking = next.brake = next.sliding = 0;
    next.fall_speed = 0;
    memset(&next.jump, 0, sizeof next.jump);
    next.airborne = !support || support == 0x8000;
    next.box_air_action = next.airborne;
    next.reported_stand = 0;
    if (next.airborne) {
        next.rider = 0;
        next.jump.active = next.jump.hurt_fall = 1;
        next.jump.phase = BFM_FOREST_JUMP_FALLING;
        next.jump.inherited_velocity = 1;
        next.jump.stride = next.jump.steering = next.speed != 0;
        if (!bfm_bedroom_motion_reenter_fall(next.ordinary, next.jump.stride)) return 0;
    } else if (support & 0x4000u) {
        next.sliding = 1;
        bfm_woods_slope_begin(&next.slope, floor.normal, 0);
        if (!bfm_bedroom_motion_reenter_slide(next.ordinary)) return 0;
    } else {
        next.speed = 0;
        if (blade_held) {
            /* The swing supplies the body key while the face continues once. */
            if (!bfm_bedroom_motion_external_face(next.ordinary, 0, 0, &next.player.pose)) return 0;
        } else {
            if (!bfm_bedroom_motion_reenter_stance(next.ordinary, tired)) return 0;
            next.reported_stand = 1;
        }
    }
    *m = next;
    return 1;
}

int bfm_village_motion_box_blade_hold(BfmVillageMotion *m, int finishing)
{
    if (!m || (!m->mountain_jump && m->woods_route.room != 0x3046) || !bfm_village_motion_grounded(m) ||
        m->grip.pole || m->water.age || m->sliding || m->talk != NO_TALK ||
        bfm_village_motion_woods_departing(m)) return 0;
    BfmVillageMotion next = *m;
    begin_field(&next);
    next.player.root.locomotion_edge = BFM_TRAVEL_NONE;
    if (!resolve_woods_footing(&next, -1, !finishing) || !publish_root(&next)) return 0;
    if (finishing && !next.airborne && !next.sliding)
        bfm_bedroom_motion_publish_upright(next.ordinary, &next.player.pose);
    *m = next;
    return 1;
}

int bfm_village_motion_woods_finish_hurt(BfmVillageMotion *m, int tired)
{
    if (!resolve_woods_footing(m, tired, 0)) return 0;
    bfm_bedroom_motion_publish_upright(m->ordinary, &m->player.pose);
    return 1;
}

int bfm_village_motion_woods_revive(BfmVillageMotion *m, int tired)
{
    return resolve_woods_footing(m, tired, 0);
}

int bfm_village_motion_woods_sliding(const BfmVillageMotion *m)
{
    return m && m->sliding;
}

int bfm_village_motion_grounded(const BfmVillageMotion *m)
{
    return m && m->player.controlled && !m->airborne;
}

void bfm_village_motion_water_bypass(BfmVillageMotion *m, int bypass)
{
    if (m) m->water.bypass = !!bypass;
}

BfmForestDustEmission bfm_village_motion_foot_dust(const BfmVillageMotion *m, unsigned actor_tick)
{
    if (!m || m->transfer || m->jump.active) return BFM_FOREST_DUST_NONE;
    if (m->woods_route.room != 0x302b && (!m->lilies || !m->player.controlled))
        return BFM_FOREST_DUST_NONE;
    if (m->foot == FOOT_START || (m->foot == FOOT_RUNNING && !(actor_tick & 7)))
        return BFM_FOREST_DUST_TRAIL;
    if (m->foot == FOOT_STOP || (m->foot == FOOT_BRAKING && !(actor_tick & 3)))
        return BFM_FOREST_DUST_BRAKE;
    return BFM_FOREST_DUST_NONE;
}

const uint8_t *bfm_village_motion_transfer(const BfmVillageMotion *m)
{
    return m ? m->transfer : NULL;
}

int bfm_village_motion_correct_contact(BfmVillageMotion *m, const int16_t xyz[3])
{
    if (!m || !xyz) return 0;
    for (unsigned i = 0; i < 3; ++i)
        m->position[i] = (int64_t)xyz[i]*Q + (m->position[i] & (Q-1));
    return publish_root(m);
}

int bfm_village_motion_resident_contact(BfmVillageMotion *m, const BfmVillageResidents *residents,
                                        unsigned field_tick, const int16_t previous[3])
{
    if (!residents || !m->player.controlled || m->talk != NO_TALK) return 1;
    int16_t xyz[3];
    memcpy(xyz, m->player.root.xyz, sizeof xyz);
    if (!bfm_village_residents_contact(residents, field_tick, previous, xyz)) return 1;
    bfm_village_residents_contact(residents, field_tick, previous, xyz);
    return bfm_village_motion_correct_contact(m, xyz);
}

int bfm_village_motion_position_q16(const BfmVillageMotion *m, int32_t xyz[3])
{
    if (!m || !xyz) return 0;
    for (unsigned i = 0; i < 3; ++i) xyz[i] = (int32_t)m->position[i];
    return 1;
}

int bfm_village_motion_staged_position(BfmVillageMotion *m, const int32_t xyz[3])
{
    if (!m || !xyz) return 0;
    for (unsigned i = 0; i < 3; ++i) m->position[i] = xyz[i];
    return publish_root(m);
}

int bfm_village_motion_staged_model(BfmVillageMotion *m, const BfmBedroomRoot *root,
    uint32_t pose, unsigned face_slot, int show_fusion, int show_lumina)
{
    if (!m || !root || pose < 16 || root->yaw >= 4096 ||
        m->player.phase != BFM_VILLAGE_CONTROL || m->talk != NO_TALK) return 0;
    const uint8_t *packet = at(&m->resident,pose-16,256);
    if (!packet) return 0;
    BfmVillageMotion next = *m;
    next.player.root = *root;
    if (!bfm_village_motion_correct_contact(&next,root->xyz) ||
        !bfm_bedroom_motion_external_face(next.ordinary,face_slot,0,&next.player.pose)) return 0;
    next.player.pose.pose = packet;
    next.player.pose.show_weapon = !!show_fusion;
    next.player.pose.show_lumina = !!show_lumina;
    next.player.events = 0;
    *m = next;
    return 1;
}

static int staged_program(BfmVillageMotion *m, const BfmBedroomRoot *root,
                                      uint32_t program, int restart, int *ended)
{
    if (!m || !root || !ended || root->yaw >= 4096 ||
        m->player.phase != BFM_VILLAGE_CONTROL || m->talk != NO_TALK) return 0;
    BfmBedroomRoot next = *root;
    if (!shadow(m, &next)) return 0;
    if (restart) {
        if (!bfm_bedroom_motion_step_then_restart(m->ordinary, program, &m->player.pose)) return 0;
    } else if (!bfm_bedroom_motion_direct(m->ordinary, program) ||
               !bfm_bedroom_motion_step(m->ordinary, BFM_BEDROOM_NONE, &m->player.pose)) {
        return 0;
    }
    m->player.root = next;
    m->player.events = 0;
    *ended = bfm_bedroom_motion_marker(m->ordinary) == BFM_BODY_END;
    return 1;
}

int bfm_village_motion_staged_program(BfmVillageMotion *m, const BfmBedroomRoot *root,
                                      uint32_t program, int *ended)
{
    return staged_program(m, root, program, 0, ended);
}

int bfm_village_motion_restart_staged_program(BfmVillageMotion *m, const BfmBedroomRoot *root,
                                              uint32_t program, int *ended)
{
    return staged_program(m, root, program, 1, ended);
}

int bfm_village_motion_finish_staged_program(BfmVillageMotion *m)
{
    return m && bfm_bedroom_motion_finish_program(m->ordinary, &m->player.pose);
}

int bfm_village_motion_staged_action(BfmVillageMotion *m, const BfmBedroomRoot *root,
                                    BfmBedroomAction action)
{
    if (!m || !root || m->player.phase != BFM_VILLAGE_CONTROL || m->talk != NO_TALK ||
        root->yaw >= 4096) return 0;
    BfmBedroomAction movement = BFM_BEDROOM_NONE;
    switch (root->locomotion_edge) {
    case BFM_TRAVEL_WALK: movement = BFM_BEDROOM_WALK; break;
    case BFM_TRAVEL_FALL: movement = BFM_BEDROOM_FALL; break;
    case BFM_TRAVEL_LAND: movement = BFM_BEDROOM_LAND; break;
    case BFM_TRAVEL_STAND: movement = BFM_BEDROOM_STAND; break;
    case BFM_TRAVEL_NONE: break;
    default: return 0;
    }
    if (action == BFM_BEDROOM_NONE) action = movement;
    BfmBedroomRoot next = *root;
    if (!shadow(m,&next) || !bfm_bedroom_motion_step(m->ordinary,action,&m->player.pose))
        return 0;
    m->player.root = next;
    m->player.events = 0;
    return 1;
}

int bfm_village_motion_staged(BfmVillageMotion *m, const BfmBedroomRoot *root)
{
    return bfm_village_motion_staged_action(m, root, BFM_BEDROOM_NONE);
}

int bfm_village_motion_porch(BfmVillageMotion *m, const BfmBedroomRoot *root, int knock)
{
    return bfm_village_motion_staged_action(m, root, knock ? BFM_BEDROOM_KNOCK : BFM_BEDROOM_NONE);
}

int bfm_village_motion_well(BfmVillageMotion *m, const BfmBedroomRoot *root, BfmBedroomAction action)
{
    if (!bfm_village_motion_staged_action(m, root, action)) return 0;
    if (action == BFM_BEDROOM_RECEIVE) m->player.events |= BFM_VILLAGE_WELL_SOUND_EVENT;
    return 1;
}

typedef enum { FIELD_PLACE, FIELD_RESUME, FIELD_AFTER_ACTOR } FieldPlacement;

static int stand_on_field(BfmVillageMotion *m, const BfmBedroomRoot *root, FieldPlacement placement)
{
    if (!m || !root || !m->player.controlled || m->talk != NO_TALK || root->yaw >= 4096)
        return 0;
    int resume = placement != FIELD_PLACE;
    BfmVillageMotion next = *m;
    for (unsigned i = 0; i < 3; ++i)
        next.position[i] = (int64_t)root->xyz[i]*Q + (resume ? (m->position[i] & (Q-1)) : 0);
    next.player.root = *root;
    next.player.root.busy = 0;
    next.player.root.locomotion_edge = BFM_TRAVEL_NONE;
    memset(&next.water,0,sizeof next.water);
    next.player.water_hidden = 0;
    next.airborne = next.landed = next.brake = next.walking = 0;
    next.rider = 0;
    memset(&next.grip,0,sizeof next.grip);
    memset(&next.jump,0,sizeof next.jump);
    next.speed = 0;
    if (!resume) next.fall_speed = 0;
    next.material = 0;
    next.player.wall_contact = 0;
    if (!publish_root(&next)) return 0;
    if (placement == FIELD_AFTER_ACTOR)
        bfm_bedroom_motion_release_draw(next.ordinary, &next.player.pose);
    else if (!bfm_bedroom_motion_step(next.ordinary,BFM_BEDROOM_ENTER,&next.player.pose)) return 0;
    *m = next;
    return 1;
}

int bfm_village_motion_place(BfmVillageMotion *m, const BfmBedroomRoot *root)
{
    return stand_on_field(m,root,FIELD_PLACE);
}

int bfm_village_motion_release_staged(BfmVillageMotion *m)
{
    return m && stand_on_field(m,&m->player.root,FIELD_RESUME);
}

int bfm_village_motion_release_staged_field(BfmVillageMotion *m)
{
    return m && stand_on_field(m,&m->player.root,FIELD_AFTER_ACTOR);
}

static int place_at_door(BfmVillageMotion *m, const uint8_t *door)
{
    BfmBedroomRoot root = {0};
    for (int i = 0; i < 3; ++i) root.xyz[i] = (int16_t)read16(door+16+2*i);
    root.yaw = (unsigned)read16(door+22) & 4095;
    root.normal[1] = -4096;
    return bfm_village_motion_place(m,&root);
}

static int set_starting_speed(BfmVillageMotion *m, uint32_t address)
{
    const uint8_t *velocity = at(&m->scene,address,12);
    if (!velocity || read32(velocity+8) < 0u-11u*Q || !read32(velocity+8)) return 0;
    m->starting_speed = (int32_t)(0u-read32(velocity+8));
    return 1;
}

static BfmVillageMotion *walking_field(const BfmVillageMotion *departed,
                                       const uint8_t *main, size_t main_bytes,
                                       const uint8_t *scene, size_t bytes,
                                       uint32_t room_address, unsigned entrance)
{
    int upper = room_address == 0x8018c794u;
    if (!scene || (!upper && room_address != 0x80192b18u)) return NULL;
    if (departed) {
        const uint8_t *door = departed->transfer;
        int bank_return = !upper && departed->room == UINT32_C(0x801910e4) &&
                          (entrance == 5 || entrance == 6);
        if (!door || read16(door) != (upper ? 0x1011 : 0x1010) ||
            (entrance != 1 && !(upper && entrance == 2) && !bank_return) ||
            door[2] != entrance) return NULL;
    } else if (!main || upper || (entrance != 3 && entrance != 4 && entrance != 10)) return NULL;
    BfmVillageMotion *m = calloc(1,sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene,bytes,scene_base};
    m->resident = departed ? departed->resident : (Region){main, main_bytes, main_base};
    m->room = room_address;
    const uint8_t *header = at(&m->scene,upper ? 0x801854b0u : 0x8018d260u,4);
    const uint8_t *room = at(&m->scene,room_address,0xcc);
    if (!header || read32(header) != room_address || !room || !load_field(m,room)) goto fail;
    const uint8_t *entry = room_door(m,room,entrance);
    if (upper) {
        m->cleft = room_door(m,room,2);
        if (!m->cleft || read32(m->cleft) != 0x02003025u) goto fail;
    } else {
        /* The west bridge meets bank entrance 1. The east bridge meets 0. */
        m->bank_doors[1] = room_door(m,room,5);
        m->bank_doors[0] = room_door(m,room,6);
        if (!m->bank_doors[0] || read32(m->bank_doors[0]) != 0x02003014u ||
            !m->bank_doors[1] || read32(m->bank_doors[1]) != 0x02013014u) goto fail;
    }
    m->outgoing = room_door(m,room,1);
    const unsigned exterior = entrance == 3 ? 0x3000 : entrance == 4 ? 0x2015 : 0x2017;
    if (!m->outgoing || read16(m->outgoing) != (upper ? 0x1010 : 0x1011) ||
        m->outgoing[2] != 1 || !entry ||
        (!departed && (read16(entry) != (int)exterior || entry[2]))) goto fail;
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    if (!set_starting_speed(m,upper ? 0x80187b0cu : 0x8018fb1cu)) goto fail;
    /* Only the seam between the village streets preserves the old foot.
     * Forest and indoor returns use their receiving room's anchor. */
    if (departed && entrance == 1) {
        if (bfm_village_motion_place(m,&departed->crossing)) return m;
    } else if (place_at_door(m,entry)) return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

static int begin_ingress(BfmVillageMotion *m, uint32_t script)
{
    const uint8_t *walk = at(&m->scene,script,12);
    if (!walk || read32(walk) != 0x8016f9c4u || read32(walk+8) != 0x8016f494u ||
        !read32(walk+4) || read32(walk+4) > 255 || m->brake_count < 12) return 0;
    m->ingress_walk = read32(walk+4);
    m->ingress = 1;
    m->player.controlled = 0;
    return 1;
}

/* The player tests the field clock before dispatching its entrance script.
 * The timed walk includes count zero, followed by a separate release update.
 * Braking can still own physical control after the script releases the clock. */
int bfm_village_motion_ingress_clock_running(const BfmVillageMotion *m)
{
    return m && m->ingress > m->ingress_walk + 2 +
        (m->woods_route.room == 0x3045 || m->woods_route.room == 0x3046);
}

static int load_bank_returns(BfmVillageMotion *m, const uint8_t *room, unsigned story)
{
    unsigned count = 0;
    const uint8_t *east = room_door(m,room,0), *west = room_door(m,room,1);
    if (!east || read32(east) != 0x00061010u || !west || read32(west) != 0x00051010u)
        return 0;
    /* Retain these per-visit descriptors across the movement transaction
     * that first queues a return. Other rooms borrow their mounted records. */
    m->bank_returns = calloc(2,sizeof *m->bank_returns);
    if (!m->bank_returns) return 0;
    unsigned target = 0x1010;
    if (story >= 900) target = 0x1094;
    else if (story >= 600) target = 0x1077;
    else if (story >= 200) target = 0x1052;
    uint32_t address = read32(room+0xb4);
    while (1) {
        const uint8_t *actor = at(&m->scene,address,20);
        if (!actor || address > UINT32_MAX-20) return 0;
        address += 20;
        unsigned type = (uint16_t)read16(actor+6);
        if (!type) break;
        if (type != 361 || read16(actor+8) != 0) continue;
        const uint8_t *door = at(&m->scene,read32(actor+16),36);
        if (count == 2 || (door != east && door != west) ||
            (count && door[2] == m->bank_returns[0].destination[2])) return 0;
        m->bank_returns[count].actor = actor;
        uint8_t *destination = m->bank_returns[count++].destination;
        memcpy(destination,door,36);
        destination[0] = (uint8_t)target;
        destination[1] = (uint8_t)(target >> 8);
    }
    return count == 2;
}

BfmVillageMotion *bfm_village_motion_bank(const uint8_t *scene, size_t bytes,
                                          const uint8_t *main, size_t main_bytes,
                                          unsigned entrance, unsigned story)
{
    if (!scene || !main || entrance > 2) return NULL;
    BfmVillageMotion *m = calloc(1,sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene,bytes,scene_base};
    m->resident = (Region){main,main_bytes,main_base};
    const uint8_t *header = at(&m->scene,0x8018c594u,4);
    if (!header || read32(header) != 0x801910e4u) goto fail;
    m->room = read32(header);
    const uint8_t *room = at(&m->scene,m->room,0xcc);
    if (!room || !load_field(m,room) || !set_starting_speed(m,0x8018ebf0u) ||
        !load_bank_returns(m,room,story) ||
        !bfm_forest_maze_bank_gate(&m->maze_gate,scene,bytes,entrance,story)) goto fail;
    const uint8_t *door = m->maze_gate.arrival;
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    if (!door || !place_at_door(m,door)) goto fail;
    m->bank_entrance = m->player.root;
    if (begin_ingress(m,0x801907ecu)) return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

static BfmVillageMotion *open_woods_motion(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned field, const uint8_t *destination)
{
    if (!scene || !main || (field != 0x3025 && field != 0x3026)) return NULL;
    int cleft = field == 0x3025;
    BfmVillageMotion *m = calloc(1,sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene,bytes,scene_base};
    m->resident = (Region){main,main_bytes,main_base};
    if (!bfm_woods_box_route_open(&m->woods_route, field, scene, bytes)) goto fail;
    const uint8_t *departure = at(&m->scene,cleft ? 0x8018b5c8u : 0x80194fe0u,16);
    if (!departure || read32(departure+8) != 0x8016f9c4u) goto fail;
    m->departure_walk = read32(departure+12);
    if (!m->departure_walk || m->departure_walk > 255) goto fail;
    m->room = cleft ? UINT32_C(0x8018ea38) : UINT32_C(0x801961e8);
    const uint8_t *room = at(&m->scene,m->room,0xcc);
    if (!room || !load_field(m,room) || !set_starting_speed(m,cleft ? 0x80189e20u : 0x80193340u))
        goto fail;

    const uint8_t *entry;
    if (destination) {
        entry = bfm_woods_box_arrival(&m->woods_route,destination);
    } else {
        entry = room_door(m,room,0);
        if (!entry || read32(entry) != 0x00021011u) goto fail;
    }
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    if (!entry || !place_at_door(m,entry)) goto fail;
    /* The hazard's last-resort placement comes from the receiving room. The
     * recorded Box destination may have been supplied in a temporary buffer. */
    const uint8_t *rescue_entry = destination && destination[2] == 0xff
        ? m->woods_route.box_destination : entry;
    m->woods_entrance = rescue_entry + 16;
    /* A recorded box has a precise foot and facing. Only ordinary arrivals walk in. */
    if (destination && destination[2] == 0xff) return m;
    m->ingress_stop_pending = cleft && destination;
    if (begin_ingress(m,cleft ? 0x8018b5b8u : 0x80194fd4u)) return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

BfmVillageMotion *bfm_village_motion_woods(const uint8_t *scene, size_t bytes,
                                           const uint8_t *main, size_t main_bytes)
{
    return open_woods_motion(scene,bytes,main,main_bytes,0x3025,NULL);
}

BfmVillageMotion *bfm_village_motion_box_field(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, const uint8_t destination[36])
{
    if (!destination) return NULL;
    return open_woods_motion(scene,bytes,main,main_bytes,(unsigned)read16(destination),destination);
}

int bfm_village_motion_trail_gate(BfmVillageMotion *m, unsigned story)
{
    if (!m || m->room != UINT32_C(0x8018ea38)) return 0;
    /* The early Cleft visit registers a 512-unit exit actor. The tagged
     * wall requests entry 0 only when the player's short foot probe also
     * intersects this live volume. */
    if (story >= 200) return 1;
    const uint8_t *room = at(&m->scene, m->room, 0xcc);
    const uint8_t *actor = room ? at(&m->scene, read32(room+0xb4), 20) : NULL;
    if (!actor || read16(actor+6) != 363) return 0;
    const uint8_t *exit = at(&m->scene, read32(actor+16), 36);
    if (!exit || read32(exit) != UINT32_C(0x00021011)) return 0;
    m->trail_gate = actor;
    m->outgoing = exit;
    return 1;
}

int bfm_village_motion_maze_contact(BfmVillageMotion *m, const int16_t endpoint[3])
{
    if (!m || !endpoint || !m->field_jumps) return 0;
    for (unsigned i = 0; i < 3; ++i)
        m->position[i] += (int64_t)(endpoint[i]-m->player.root.xyz[i])*Q;
    return publish_root(m);
}

const BfmForestMazeGate *bfm_village_motion_maze_gate(const BfmVillageMotion *m)
{
    return m ? &m->maze_gate : NULL;
}

const BfmVillageGround *bfm_village_motion_ground(const BfmVillageMotion *m)
{
    return m ? m->ground : NULL;
}

BfmVillageMotion *bfm_village_motion_arrive(const BfmVillageMotion *departed, const uint8_t *scene,
                                            size_t bytes, uint32_t room_address)
{
    const uint8_t *door = bfm_village_motion_transfer(departed);
    return door ? walking_field(departed, NULL, 0, scene, bytes, room_address, door[2]) : NULL;
}

BfmVillageMotion *bfm_village_motion_return_to_street(const uint8_t *scene, size_t bytes,
                                    uint32_t room_address, const uint8_t *main, size_t main_bytes,
                                    unsigned entrance)
{
    if (room_address != UINT32_C(0x80190780))
        return walking_field(NULL, main, main_bytes, scene, bytes, room_address, entrance);
    if (!scene || !main || (entrance != 10 && entrance != 15)) return NULL;
    BfmVillageMotion *m = calloc(1,sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene,bytes,scene_base};
    m->resident = (Region){main,main_bytes,main_base};
    m->room = room_address;
    const uint8_t *binding = at(&m->scene, UINT32_C(0x8018aa0c), 4);
    const uint8_t *room = at(&m->scene, room_address, 0xcc);
    if (!binding || read32(binding) != room_address || !room || !load_field(m,room) ||
        !set_starting_speed(m,UINT32_C(0x8018d1c0))) goto fail;
    m->outgoing = room_door(m,room,10);
    if (!m->outgoing || read32(m->outgoing) != UINT32_C(0x06002058)) goto fail;
    const uint8_t *entry = m->outgoing;
    if (entrance == 15) {
        entry = room_door(m,room,15);
        if (!entry || read32(entry) != UINT32_C(0x02003043)) goto fail;
    }
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    if (place_at_door(m,entry)) return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

BfmVillageMotion *bfm_village_motion_resume(const uint8_t *scene, size_t bytes,
                                            uint32_t room_address,
                                            const uint8_t *main, size_t main_bytes)
{
    return bfm_village_motion_return_to_street(scene,bytes,room_address,main,main_bytes,10);
}

int bfm_village_motion_continue(BfmVillageMotion *m)
{
    if (!m || m->pending || (m->player.phase != BFM_VILLAGE_WAIT_MUSIC &&
        m->player.phase != BFM_VILLAGE_FIRST_MESSAGE && m->player.phase != BFM_VILLAGE_SECOND_MESSAGE))
        return 0;
    m->pending = 1;
    return 1;
}

int bfm_village_motion_begin_talk(BfmVillageMotion *m, const int16_t resident[3], unsigned yaw)
{
    if (!m || !resident || yaw >= 4096 || !m->player.controlled || m->talk != NO_TALK)
        return 0;
    const uint8_t *approach = at(&m->scene, 0x801898b4u, 16);
    if (!approach || read32(approach) != 4 || read32(approach+8) != 10) return 0;
    int local_x = read16(approach+4), local_z = read16(approach+6);
    int sine = (int)lround(sin(yaw*angle_unit)*4096);
    int cosine = (int)lround(cos(yaw*angle_unit)*4096);
    int x = resident[0] + (int)floor((local_x*cosine + local_z*sine)/4096.0);
    int z = resident[2] + (int)floor((local_z*cosine - local_x*sine)/4096.0);
    if (x < INT16_MIN || x > INT16_MAX || z < INT16_MIN || z > INT16_MAX) return 0;
    const int16_t target[3] = {(int16_t)x, resident[1], (int16_t)z};
    return bfm_village_motion_begin_talk_at(m, resident, target);
}

int bfm_village_motion_begin_talk_at(BfmVillageMotion *m, const int16_t resident[3],
                                     const int16_t approach[3])
{
    if (!m || !resident || !approach || !m->player.controlled || m->talk != NO_TALK)
        return 0;
    m->resident_x = resident[0];
    m->resident_z = resident[2];
    m->talk_x = approach[0];
    m->talk_z = approach[2];
    m->walking = 0;
    m->talk_dispatch = 2;
    m->talk = TALK_APPROACH;
    m->player.controlled = 0;
    m->player.root.busy = 1;
    return 1;
}

int bfm_village_motion_begin_talk_hold(BfmVillageMotion *m)
{
    if (!m || !m->player.controlled || m->talk != NO_TALK) return 0;
    if ((m->walking || m->brake) &&
        !bfm_bedroom_motion_step(m->ordinary, BFM_BEDROOM_STAND, &m->player.pose)) return 0;
    m->walking = m->brake = 0;
    m->talk_dispatch = 0;
    m->talk = TALK_HOLD;
    m->player.controlled = 0;
    m->player.root.busy = 1;
    return 1;
}

int bfm_village_motion_begin_read(BfmVillageMotion *m, const int16_t point[3])
{
    if (!m || !point || !m->player.controlled || m->talk != NO_TALK)
        return 0;
    if ((m->walking || m->brake) &&
        !bfm_bedroom_motion_step(m->ordinary,BFM_BEDROOM_STAND,&m->player.pose)) return 0;
    m->turn = (unsigned)bfm_waypoint_heading(m->player.root.xyz[0]-point[0],m->player.root.xyz[2]-point[2]);
    m->walking = m->brake = 0;
    m->talk_dispatch = 2;
    m->talk = READ_SIGN;
    m->player.controlled = 0;
    m->player.root.busy = 1;
    return 1;
}

int bfm_village_motion_end_read(BfmVillageMotion *m)
{
    if (!m || m->talk != READ_SIGN) return 0;
    m->talk = NO_TALK;
    m->player.controlled = 1;
    m->player.root.busy = 0;
    return 1;
}

int bfm_village_motion_talk_ready(const BfmVillageMotion *m)
{
    return m && m->talk == TALK_HOLD;
}

int bfm_village_motion_end_talk(BfmVillageMotion *m)
{
    if (!bfm_village_motion_talk_ready(m)) return 0;
    m->talk = NO_TALK;
    m->player.controlled = 1;
    m->player.root.busy = 0;
    return 1;
}

const BfmVillagePlayer *bfm_village_motion_player(const BfmVillageMotion *m)
{
    return m ? &m->player : NULL;
}

int bfm_village_motion_face(const BfmVillageMotion *m, uint16_t tiles[512][1024])
{
    return m && tiles && bfm_bedroom_motion_face(m->ordinary,tiles);
}

int bfm_village_motion_lower_orb(const BfmVillageMotion *m, unsigned orb, uint16_t tiles[512][1024])
{
    if (m && m->room == UINT32_C(0x80190780))
        return bfm_field_blade_palette(m->ordinary, UINT32_C(0x8018e378), orb, tiles);
    return m && bfm_grillin_blade_palette(m->ordinary, orb, tiles);
}

int bfm_village_motion_mine_orb(const BfmVillageMotion *m, unsigned orb, uint16_t tiles[512][1024])
{
    if (m && m->woods_route.room == 0x3046)
        return bfm_field_blade_palette(m->ordinary, UINT32_C(0x80185534), orb, tiles);
    if (m && m->woods_route.room == 0x3045)
        return bfm_field_blade_palette(m->ordinary, UINT32_C(0x80181b14), orb, tiles);
    if (m && m->woods_route.room == 0x3044)
        return bfm_field_blade_palette(m->ordinary, UINT32_C(0x80184468), orb, tiles);
    return m && m->room == UINT32_C(0x8018615c) &&
           bfm_field_blade_palette(m->ordinary, UINT32_C(0x80185c70), orb, tiles);
}

int bfm_village_motion_twinpeak_orb(const BfmVillageMotion *m, unsigned orb, uint16_t tiles[512][1024])
{
    return m && bfm_field_blade_palette(m->ordinary, UINT32_C(0x801944f8), orb, tiles);
}

int bfm_village_motion_valley_orb(const BfmVillageMotion *m, unsigned orb, uint16_t tiles[512][1024])
{
    return m && m->woods_route.room == 0x302b &&
           bfm_field_blade_palette(m->ordinary, UINT32_C(0x80185b30), orb, tiles);
}

void bfm_village_motion_free(BfmVillageMotion *m)
{
    if (m) {
        bfm_bedroom_motion_free(m->ordinary);
        bfm_village_ground_free(m->ground);
        free(m->mountain_jump);
        free(m->field_jumps);
        free(m->lake_blade_data);
        free(m->bank_returns);
    }
    free(m);
}

static int load_field_jumps(BfmVillageMotion *m, const uint8_t *room, const MusashiDiscMedia *disc)
{
    m->field_jumps = malloc(sizeof *m->field_jumps);
    return m->field_jumps && bfm_woods_jump_pose_load(m->field_jumps, disc, read32(room+0x4c));
}

BfmVillageMotion *bfm_village_motion_mine(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, unsigned story, const MusashiDiscMedia *disc)
{
    if (!scene || !main || !disc || entrance > 1) return NULL;
    BfmVillageMotion *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene, bytes, scene_base};
    m->resident = (Region){main, main_bytes, main_base};
    const uint8_t *binding = at(&m->scene, UINT32_C(0x8018245c), 4);
    if (!binding || read32(binding) != UINT32_C(0x8018615c)) goto fail;
    m->room = read32(binding);
    const uint8_t *room = at(&m->scene, m->room, 0xcc);
    if (!room || !load_field(m, room) || !set_starting_speed(m, UINT32_C(0x80184ab8)) ||
        !load_field_jumps(m, room, disc) || !bfm_mine_pole_load(&m->poles, scene, bytes) ||
        !bfm_woods_pole_pose_load_bank(&m->pole_pose, main, main_bytes, read32(room+0x4c))) goto fail;
    m->pole_jump_speed = pole_takeoff_speed(m, UINT32_C(0x80184af8));
    if (!m->pole_jump_speed) goto fail;
    const uint8_t *west = room_door(m, room, 0);
    const uint8_t *east = room_door(m, room, 1);
    const uint8_t *sensor = at(&m->scene, read32(room+0xb4), 40);
    if (!west || read32(west) != UINT32_C(0x000f1010) || !sensor ||
        read16(sensor+6) != 442 ||
        at(&m->scene, read32(sensor+16), 36) != west) goto fail;
    /* Only the outgoing actor changes its route for the current chapter.
     * Ingress still takes the position and facing from the room's own row. */
    const uint8_t *west_exit = west;
    if (story >= 200) {
        uint32_t pairs = story < 600 ? UINT32_C(0x80186080) :
                         story < 900 ? UINT32_C(0x80186090) : UINT32_C(0x801860a0);
        const uint8_t *mapping = at(&m->scene, pairs, 8);
        if (!mapping || at(&m->scene, read32(mapping), 36) != west) goto fail;
        west_exit = at(&m->scene, read32(mapping+4), 36);
        if (!west_exit) goto fail;
    }
    const uint8_t *east_actor = sensor + 20;
    if (!east || read32(east) != 0x3044 || read16(east_actor+6) != 126 ||
        at(&m->scene, read32(east_actor+16), 36) != east) goto fail;
    m->woods_route = (BfmWoodsBoxRoute){.room=0x3043, .passage=west_exit, .passage_actor=sensor,
                                       .valley_passage=east, .valley_actor=east_actor};
    const uint8_t *departure = at(&m->scene, UINT32_C(0x80185fd8), 20);
    if (!departure || read32(departure) != UINT32_C(0x801706ac) ||
        read32(departure+4) != UINT32_C(0x801704b0) || read32(departure+8) != UINT32_C(0x8016f9c4) ||
        read32(departure+16) != UINT32_C(0x8016f4f4)) goto fail;
    m->departure_walk = read32(departure+12);
    if (!m->departure_walk || m->departure_walk > 255) goto fail;
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    m->ingress_stop_pending = 1;
    if (!place_at_door(m, entrance ? east : west)) goto fail;
    m->bank_entrance = m->player.root;
    if (begin_ingress(m, UINT32_C(0x80185fcc))) return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

static int register_mine_gates(BfmVillageMotion *m, const uint8_t *room, unsigned room_id)
{
    const uint8_t *actors = at(&m->scene, read32(room + 0xb4), 40);
    const uint8_t *west = room_door(m, room, 0), *east = room_door(m, room, 1);
    if (!actors || !west || !east || read16(actors + 6) != 126 || read16(actors + 26) != 126 ||
        read32(west) != (UINT32_C(0x10000) + room_id - 1u) || read32(east) != room_id + 1u ||
        at(&m->scene, read32(actors + 16), 36) != west ||
        at(&m->scene, read32(actors + 36), 36) != east) return 0;
    m->woods_route = (BfmWoodsBoxRoute){.room=room_id, .passage=west, .passage_actor=actors,
                                       .valley_passage=east, .valley_actor=actors+20};
    return 1;
}

static int place_shaft_checkpoint(BfmVillageMotion *m, const uint8_t *room, const uint8_t saved[36])
{
    const uint8_t *actors = at(&m->scene, read32(room + 0xb4), 8 * 20);
    const uint8_t *box = actors ? actors + 7 * 20 : NULL;
    const uint8_t *release = at(&m->resident, UINT32_C(0x800d3aac), 4);
    if (!box || read16(box + 6) != 483 || read32(box + 16) != UINT32_C(0x80181d4c) ||
        !release || read32(release) != UINT32_C(0x8016f494)) return 0;
    const uint8_t *row = at(&m->scene, read32(box + 16), 36);
    if (!row || read32(row) != UINT32_C(0x00ff3045) || read32(row + 8) != UINT32_C(0x800d2f0c) ||
        memcmp(row, saved, 36) || !place_at_door(m, row)) return 0;
    m->box_release_fields = 2;
    m->player.controlled = 0;
    return 1;
}

static BfmVillageMotion *open_mine_passage(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned room_id, unsigned entrance, const uint8_t *saved,
    const MusashiDiscMedia *disc)
{
    static const struct {
        uint32_t binding, room, stride, entry_walk, exit_walk;
    } rooms[] = {
        {0x80180c54, 0x80184870, 0x801832b0, 0x801846fc, 0x80184708},
        {0x8017e300, 0x80181f28, 0x8018095c, 0x80181e30, 0x80181e3c},
        {0x80181d20, 0x80185a50, 0x8018437c, 0x80185960, 0x8018596c}
    };
    if (!scene || !main || !disc || entrance > 1 || room_id < 0x3044 || room_id > 0x3046) return NULL;
    unsigned field = room_id - 0x3044;
    BfmVillageMotion *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene, bytes, scene_base};
    m->resident = (Region){main, main_bytes, main_base};
    const uint8_t *binding = at(&m->scene, rooms[field].binding, 4);
    if (!binding || read32(binding) != rooms[field].room) goto fail;
    m->room = read32(binding);
    const uint8_t *room = at(&m->scene, m->room, 0xcc);
    if (!room || !load_field(m, room) || !set_starting_speed(m, rooms[field].stride) ||
        !load_field_jumps(m, room, disc)) goto fail;
    if (room_id == 0x3045) {
        if (!bfm_village_mine_shaft_gates_open(&m->shaft_gates, scene, bytes, m->room) ||
            read32(m->shaft_gates.gate[0].door) != UINT32_C(0x00013044) ||
            read32(m->shaft_gates.gate[1].door) != UINT32_C(0x00003046)) goto fail;
        m->woods_route.room = 0x3045;
    } else if (!register_mine_gates(m, room, room_id)) goto fail;
    if (room_id == 0x3046) {
        m->lake_blade_data = malloc(sizeof *m->lake_blade_data);
        if (!m->lake_blade_data ||
            !bfm_lake_jump_blade_load(m->lake_blade_data, disc, scene, bytes)) goto fail;
    }
    const uint8_t *departure = at(&m->scene, rooms[field].exit_walk, 20);
    if (!departure || read32(departure) != UINT32_C(0x801706ac) ||
        read32(departure + 4) != UINT32_C(0x801704b0) || read32(departure + 8) != UINT32_C(0x8016f9c4) ||
        read32(departure + 16) != UINT32_C(0x8016f4f4)) goto fail;
    m->departure_walk = read32(departure + 12);
    if (!m->departure_walk || m->departure_walk > 255) goto fail;
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    if (saved) {
        if (!place_shaft_checkpoint(m, room, saved)) goto fail;
    } else {
        const uint8_t *door = room_door(m, room, entrance);
        if (!door || !place_at_door(m, door) || !begin_ingress(m, rooms[field].entry_walk)) goto fail;
        m->ingress_stop_pending = room_id != 0x3044;
    }
    m->bank_entrance = m->player.root;
    return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

BfmVillageMotion *bfm_village_motion_switchback(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, const MusashiDiscMedia *disc)
{
    return open_mine_passage(scene, bytes, main, main_bytes, 0x3044, entrance, NULL, disc);
}

BfmVillageMotion *bfm_village_motion_mine_shaft(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, const MusashiDiscMedia *disc)
{
    return open_mine_passage(scene, bytes, main, main_bytes, 0x3045, entrance, NULL, disc);
}

BfmVillageMotion *bfm_village_motion_mine_lake(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, const MusashiDiscMedia *disc)
{
    return open_mine_passage(scene, bytes, main, main_bytes, 0x3046, entrance, NULL, disc);
}

void bfm_village_motion_mine_shaft_lake_exit(BfmVillageMotion *m, int enabled)
{
    if (m && m->woods_route.room == 0x3045) m->shaft_lake_open = enabled != 0;
}

void bfm_village_motion_mine_lake_fields(BfmVillageMotion *m,
    const BfmVillageMineLakeFields *fields, unsigned actor_tick)
{
    if (!m || m->woods_route.room != 0x3046) return;
    m->lake_fields = fields;
    m->lake_field_tick = actor_tick;
}

void bfm_village_motion_mine_lake_begin_field(BfmVillageMotion *m, const int16_t current[3])
{
    if (!m || m->woods_route.room != 0x3046) return;
    for (unsigned axis = 0; axis < 3; ++axis)
        m->mine_action_foot[axis] = (int16_t)high(m->position[axis]);
    m->lake_blade_frame = (BfmShaftBladeFrame){0};
    m->lake_current_posted = current != NULL;
    if (current) memcpy(m->lake_current, current, sizeof m->lake_current);
}

BfmVillageMotion *bfm_village_motion_mine_shaft_saved(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, const uint8_t destination[36], const MusashiDiscMedia *disc)
{
    if (!destination || read32(destination) != UINT32_C(0x00ff3045)) return NULL;
    return open_mine_passage(scene, bytes, main, main_bytes, 0x3045, 0, destination, disc);
}

BfmVillageMotion *bfm_village_motion_valley(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, const uint8_t destination[36], const MusashiDiscMedia *disc)
{
    if (!scene || !main || !destination || read32(destination) != 0x302b || !disc) return NULL;
    BfmVillageMotion *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene, bytes, scene_base};
    m->resident = (Region){main, main_bytes, main_base};
    const uint8_t *binding = at(&m->scene, UINT32_C(0x8018231c), 4);
    if (!binding || read32(binding) != UINT32_C(0x80186620)) goto fail;
    m->room = read32(binding);
    const uint8_t *room = at(&m->scene, m->room, 0xcc);
    if (!room || !load_field(m, room) || !set_starting_speed(m, UINT32_C(0x80184978)) ||
        !load_field_jumps(m, room, disc)) goto fail;

    /* Entrance zero faces into the valley from the Cleft-side gate. The same
     * sector supplies the gate volume and the reverse room descriptor. */
    const uint8_t *gate = at(&m->scene, read32(room+0xb4), 20);
    const uint8_t *door = room_door(m, room, 0);
    if (!gate || read16(gate+6) != 126 || !door || read32(door) != UINT32_C(0x00013025) ||
        at(&m->scene, read32(gate+16), 36) != door) goto fail;
    m->woods_route = (BfmWoodsBoxRoute){.room=0x302b, .passage=door, .passage_actor=gate};
    const uint8_t *departure = at(&m->scene, UINT32_C(0x80185fa4), 16);
    if (!departure || read32(departure+8) != UINT32_C(0x8016f9c4)) goto fail;
    m->departure_walk = read32(departure+12);
    if (!m->departure_walk || m->departure_walk > 255) goto fail;
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    m->ingress_stop_pending = 1;
    if (!place_at_door(m, door)) goto fail;
    m->bank_entrance = m->player.root;
    if (begin_ingress(m, UINT32_C(0x80185f98))) return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

BfmVillageMotion *bfm_village_motion_maze(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, const MusashiDiscMedia *disc)
{
    if (!scene || !main || !disc) return NULL;
    BfmVillageMotion *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene, bytes, scene_base};
    m->resident = (Region){main, main_bytes, main_base};
    if (!bfm_forest_maze_return_gate(&m->maze_gate, scene, bytes, entrance)) goto fail;
    m->room = UINT32_C(0x8018c890);
    const uint8_t *room = at(&m->scene, m->room, 0xcc);
    if (!room || !load_field(m, room) || !set_starting_speed(m, 0x801899e8u)) goto fail;
    if (!load_field_jumps(m, room, disc)) goto fail;
    /* Both south entries install the same walk-in, even on a later visit
     * with different actors. Release the normal pad after its braking fields. */
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    if (place_at_door(m, m->maze_gate.arrival) && begin_ingress(m, 0x8018bbf8u)) return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

static int step_field_with_jump(BfmVillageMotion *m, int x, int y, int camera_yaw,
                                int cross_pressed, int cross_held, BfmGameplayRun *run)
{
    if (!m || !m->field_jumps || x < -127 || x > 127 || y < -127 || y > 127) return 0;
    BfmVillageMotion next = *m;
    next.jump.pressed = cross_pressed;
    next.jump.held = cross_held;
    if (!advance(&next,x,y,camera_yaw & 4095,run,1)) return 0;
    next.jump.previous_x = x;
    next.jump.previous_y = y;
    *m = next;
    return 1;
}

int bfm_village_motion_mine_shaft_box(BfmVillageMotion *m, const BfmWoods3026BoxHit *box,
                                      unsigned tick, const int16_t previous[3])
{
    if (!m || m->woods_route.room != 0x3045 || !box || !previous) return 0;
    if (!m->player.controlled) return 1;
    int16_t foot[3];
    memcpy(foot, m->player.root.xyz, sizeof foot);
    return !bfm_woods_3026_box_body(box, tick, previous, foot) ||
           bfm_village_motion_correct_contact(m, foot);
}

void bfm_village_motion_mine_shaft_begin_field(BfmVillageMotion *m, BfmMineShaftFan *fan,
                                               unsigned tick)
{
    if (!m || m->woods_route.room != 0x3045) return;
    m->mine_fan = fan;
    m->shaft_tick = tick;
    for (unsigned axis = 0; axis < 3; ++axis)
        m->mine_action_foot[axis] = (int16_t)high(m->position[axis]);
}

int bfm_village_motion_mine_shaft_turn_fan(BfmVillageMotion *m, BfmMineShaftFanFrame *frame)
{
    if (!m || !m->mine_fan || !frame) return 0;
    int32_t foot[3];
    for (unsigned axis = 0; axis < 3; ++axis) foot[axis] = (int32_t)m->position[axis];
    /* Jump and unsupported fall retain the accumulated vertical wind. The
     * landing keys still belong to their jump/fall action until control
     * selects walking or standing again. */
    unsigned action = 0;
    if (m->jump.active) action = m->jump.hurt_fall ? 3 : 2;
    else if (m->airborne || m->landed) action = 3;
    *frame = bfm_mine_shaft_fan_step(m->mine_fan, action, (int)m->rider-1, foot, &m->fall_speed);
    m->player.root.yaw = (m->player.root.yaw + (unsigned)frame->carry_yaw) & 4095;
    BfmBedroomRoot shadow = m->player.root;
    for (unsigned axis = 0; axis < 3; ++axis) {
        m->position[axis] = foot[axis];
        shadow.xyz[axis] = (int16_t)high(foot[axis]);
    }
    if (!bfm_bedroom_travel_air_shadow(m->scene.data, m->scene.bytes, m->room, &shadow)) return 0;
    bfm_mine_shaft_fan_shadow(m->mine_fan, shadow.xyz, &shadow);
    memcpy(m->player.root.shadow_xyz, shadow.shadow_xyz, sizeof shadow.shadow_xyz);
    memcpy(m->player.root.shadow_normal, shadow.shadow_normal, sizeof shadow.shadow_normal);
    m->player.root.shadow_visible = shadow.shadow_visible;
    return 1;
}

void bfm_village_motion_mine_begin_field(BfmVillageMotion *m, BfmVillageMineWheels *wheels)
{
    m->mine_wheels = wheels;
    for (unsigned axis = 0; axis < 3; ++axis)
        m->mine_action_foot[axis] = (int16_t)high(m->position[axis]);
}

int bfm_village_motion_mine_rider(const BfmVillageMotion *m)
{
    return m && m->mine_wheels ? (int)m->rider - 1 : -1;
}

int bfm_village_motion_mine_turn_wheels(BfmVillageMotion *m, int gain[3])
{
    int32_t foot[3];
    for (unsigned axis = 0; axis < 3; ++axis) foot[axis] = (int32_t)m->position[axis];
    BfmMineWheelAudio audio = bfm_village_mine_wheels_step(m->mine_wheels,
        m->player.root.xyz[0], bfm_village_motion_mine_rider(m), foot);
    memcpy(gain, audio.gain, sizeof audio.gain);
    BfmBedroomRoot shadow = m->player.root;
    for (unsigned axis = 0; axis < 3; ++axis) {
        m->position[axis] = foot[axis];
        shadow.xyz[axis] = (int16_t)high(foot[axis]);
    }
    /* The body and camera already used the published player root. This
     * later shadow pass follows the raw foot after the wheels carried it. */
    if (!bfm_bedroom_travel_air_shadow(m->scene.data, m->scene.bytes, m->room, &shadow)) return 0;
    bfm_village_mine_decks_shadow(m->mine_wheels, shadow.xyz, &shadow);
    memcpy(m->player.root.shadow_xyz, shadow.shadow_xyz, sizeof shadow.shadow_xyz);
    memcpy(m->player.root.shadow_normal, shadow.shadow_normal, sizeof shadow.shadow_normal);
    m->player.root.shadow_visible = shadow.shadow_visible;
    return 1;
}

void bfm_village_motion_mine_props(BfmVillageMotion *m, const BfmVillageMineProps *props, unsigned tick)
{
    m->mine_props = props;
    m->mine_props_tick = tick;
}

unsigned bfm_village_motion_mine_blade_slot(const BfmVillageMotion *m)
{
    if (!m || m->woods_route.room != 0x3046 || !m->player.controlled || m->airborne ||
        m->sliding || m->talk != NO_TALK || bfm_village_motion_woods_departing(m)) return 0;
    return m->walking ? 2 : 1;
}

int bfm_village_motion_mine_blade_hold(BfmVillageMotion *m, int finishing, int redirect_yaw)
{
    if (!m || m->woods_route.room != 0x3046 || !bfm_village_motion_box_blade_hold(m, finishing)) return 0;
    if (redirect_yaw >= 0) m->player.root.yaw = (unsigned)redirect_yaw & 4095u;
    return 1;
}

int bfm_village_motion_switchback_stance(BfmVillageMotion *m, unsigned facing,
    unsigned stride_yaw, int moving, int tired, int *supported)
{
    if (!m || !supported || !m->player.controlled ||
        (m->woods_route.room != 0x3044 && m->woods_route.room != 0x3046)) return 0;
    BfmVillageMotion next = *m;
    begin_field(&next);
    *supported = 0;
    if (!next.transfer) {
        BfmWoodsMotionContact contact = {0};
        /* The pad bearing selects a local stride. The separate model bearing
         * projects that stride back into the room before the floor sweep. */
        ForestPlayerState stride = {0};
        stride.field_2c.word = moving ? -4*Q : 0;
        if (!bfm_forest_level_velocity(&stride, 0, (int16_t)(stride_yaw-facing), 0)) return 0;
        stride.field_24.word = (int32_t)stride.velocity[0];
        stride.field_2c.word = (int32_t)stride.velocity[2];
        if (!bfm_forest_level_velocity(&stride, 0, (int16_t)facing, 0) ||
            !displace(&next, stride.velocity, 1, &contact, NULL)) return 0;
        next.player.root.yaw = facing & 4095u;
        next.landed = next.brake = next.walking = 0;
        next.speed = next.fall_speed = 0;
        *supported = !contact.lost_support && !(contact.slope & 0x4000u);
        if (!publish_root(&next)) return 0;
        if (!*supported && !bfm_village_motion_woods_finish_hurt(&next, tired)) return 0;
    }
    *m = next;
    return 1;
}

int bfm_village_motion_step_mine_lake_pad(BfmVillageMotion *m, BfmFieldInput input,
    int camera_yaw, const uint8_t *progress, BfmGameplayRun *run)
{
    if (!m || m->woods_route.room != 0x3046) return 0;
    BfmVillageMotion next = *m;
    next.lake_pad = input.movement_pad == BFM_FIELD_PAD_ANALOG
        ? BFM_FIELD_PAD_ANALOG : BFM_FIELD_PAD_DIGITAL;
    if (next.lake_pad == BFM_FIELD_PAD_DIGITAL && input.charge_direction.sampled) {
        input.x = input.charge_direction.x;
        input.y = input.charge_direction.y;
    }
    next.lake_tired = progress && bfm_field_tired_stance(progress);
    int reserve = progress ? read16(progress + 0x26) : 64;
    int wakefulness = progress ? read16(progress + 0x2e) : 48;
    next.lake_speed_limit = reserve < 32 || wakefulness < 32 ? 4*Q :
                            reserve < 64 || wakefulness < 48 ? 8*Q : 11*Q;
    if (!step_field_with_jump(&next, input.x, input.y, camera_yaw,
                              input.cross_pressed, input.cross_held, run)) return 0;
    next.lake_pad = BFM_FIELD_PAD_UNSPECIFIED;
    next.lake_tired = 0;
    *m = next;
    return 1;
}

int bfm_village_motion_step_mine_field(BfmVillageMotion *m, int x, int y, int camera_yaw,
                                       int cross_pressed, int cross_held, BfmGameplayRun *run)
{
    if (m && m->woods_route.room == 0x3046) {
        BfmFieldInput input = {.x = x, .y = y, .cross_pressed = cross_pressed,
                               .cross_held = cross_held, .movement_pad = BFM_FIELD_PAD_DIGITAL};
        return bfm_village_motion_step_mine_lake_pad(m, input, camera_yaw,
            run && run->has_player ? run->player.record : NULL, run);
    }
    return m && m->woods_route.room >= 0x3043 && m->woods_route.room <= 0x3045 &&
           step_field_with_jump(m,x,y,camera_yaw,cross_pressed,cross_held,run);
}

int bfm_village_motion_step_mine_lake_field(BfmVillageMotion *m, BfmFieldInput input,
    int camera_yaw, const BfmSaveRecord *live, BfmGameplayRun *run)
{
    if (!m || m->woods_route.room != 0x3046) return 0;
    BfmVillageMotion next = *m;
    next.lake_square = input.square_pressed;
    next.lake_blade_flags = (next.player.pose.hide_fusion ? 4u : 0u) |
                            (live && (live->progress[0x23] & 0x80) ? 0x400u : 0u);
    next.lake_blade_frame = (BfmShaftBladeFrame){0};
    if (!bfm_village_motion_step_mine_lake_pad(&next, input, camera_yaw,
                                               live ? live->progress : NULL, run)) return 0;
    next.lake_square = 0;
    *m = next;
    return 1;
}

const BfmShaftBladeFrame *bfm_village_motion_mine_shaft_blade(const BfmVillageMotion *m)
{
    return m && m->lake_blade_frame.event ? &m->lake_blade_frame : NULL;
}

int bfm_village_motion_step_maze_field(BfmVillageMotion *m, int x, int y, int camera_yaw,
                                       int cross_pressed, int cross_held, BfmGameplayRun *run)
{
    return m && m->maze_gate.arrival &&
           step_field_with_jump(m,x,y,camera_yaw,cross_pressed,cross_held,run);
}

int bfm_village_motion_can_examine(const BfmVillageMotion *m)
{
    /* Walk entry installs the active walker on the starting field. A run
     * brake is a body clip inside the active standing action. Both test a
     * fresh Square before moving on their next field. */
    return m && m->player.phase == BFM_VILLAGE_CONTROL && m->player.controlled && !m->transfer &&
           m->talk == NO_TALK && !m->ingress && !m->airborne && !m->landed && !m->sliding;
}

void bfm_village_motion_mine_gate(BfmVillageMotion *motion, const BfmVillageMineGate *gate, unsigned tick)
{
    motion->mine_gate = gate;
    motion->mine_gate_tick = tick;
}

void bfm_village_motion_valley_obstacles(BfmVillageMotion *motion,
    const BfmValleyBincho *bincho, const BfmValleyGateDoor *door)
{
    motion->valley_bincho = bincho;
    motion->valley_door = door;
}

void bfm_village_motion_valley_boulders(BfmVillageMotion *motion,
    const BfmHellsValleyBoulders *rocks, unsigned actor_tick)
{
    motion->valley_rocks = rocks;
    motion->valley_tick = actor_tick;
}

int bfm_village_motion_step_valley_field(BfmVillageMotion *m, int x, int y, int camera_yaw,
                                         int cross_pressed, int cross_held, BfmGameplayRun *run)
{
    return m && m->woods_route.room == 0x302b &&
           step_field_with_jump(m,x,y,camera_yaw,cross_pressed,cross_held,run);
}

int bfm_village_motion_maze_departing(BfmVillageMotion *m)
{
    if (!m || !m->transfer || m->transfer != m->maze_gate.destination) return 0;
    const uint8_t *walk = at(&m->scene, m->field_jumps ? 0x8018bc0cu : 0x80190804u, 8);
    if (!walk || read32(walk) != 0x8016f9c4u || read32(walk+4) > 255) return 0;

    BfmVillageMotion next = *m;
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    next.player.events = 0;
    next.player.controlled = 0;
    next.foot = FOOT_QUIET;
    if (next.maze_departure_field == 0) {
        /* The departure script takes ownership of the hero and first resets
         * the tracking camera. Its second command faces the struck actor's
         * placed heading, before it synthesizes forward pad input. */
        next.walking = next.brake = next.sliding = 0;
        next.jump.active = 0;
        action = BFM_BEDROOM_STAND;
    } else if (next.maze_departure_field == 1) {
        next.player.root.yaw = (unsigned)read16(next.maze_gate.actor+14) & 4095;
    } else if (next.maze_departure_field == 2) {
        next.walking = 1;
        action = BFM_BEDROOM_WALK;
    }
    unsigned length = read32(walk+4);
    int moving = next.maze_departure_field > 2 && next.maze_departure_field <= 2 + length;
    if (next.airborne && next.fall_speed < 31*Q) next.fall_speed += GRAVITY;
    if ((moving || next.airborne) && !move(&next,moving ? 11*Q : 0,
                                          next.airborne ? next.fall_speed : 0,1)) return 0;
    if (next.maze_departure_field <= 2 + length) ++next.maze_departure_field;
    if (!publish_root(&next) ||
        !bfm_bedroom_motion_step(next.ordinary,action,&next.player.pose)) return 0;
    *m = next;
    return 1;
}

int bfm_village_motion_maze_resume_hop(BfmVillageMotion *m, BfmForestBankHop *hop,
                                       BfmForestBankHopPose *poses)
{
    if (!m || !m->field_jumps || !m->player.controlled || !hop || !poses || m->transfer) return 0;
    /* The entry terminator starts a fresh arc in place. Its first integrator
     * update and timer tick belong to the following unscripted field. */
    if (!bfm_forest_bank_hop_pose_step(poses,1,0,m->ordinary,1,&m->player.pose)) return 0;
    hop->active = 1;
    hop->launch_y = m->player.root.xyz[1];
    bfm_forest_bank_hop_rebound(hop);
    m->walking = m->brake = m->speed = m->landed = 0;
    m->airborne = 1;
    m->fall_speed = hop->vertical_q16;
    return 1;
}

int bfm_village_motion_rootrick_begin(BfmVillageMotion *m, BfmRootrickHero *hero,
                                      const int64_t staged_position[3])
{
    if (!m || !hero || !staged_position || m->room != UINT32_C(0x801961e8) || !m->ordinary || !m->ground ||
        m->player.phase != BFM_VILLAGE_CONTROL || !m->player.controlled) return 0;
    for (unsigned i = 0; i < 3; ++i)
        if (high(staged_position[i]) != m->player.root.xyz[i]) return 0;
    memcpy(m->position,staged_position,sizeof m->position);
    bfm_rootrick_hero_begin(hero);
    return 1;
}

int bfm_village_motion_rootrick_step(BfmVillageMotion *m, BfmRootrickHero *hero,
    const BfmRootrickHeroObstacle *before, const BfmRootrickHeroObstacle *after,
    BfmFieldInput input, int camera_yaw,
    unsigned field_tick, int can_climb, int32_t *player_y_q16)
{
    if (!m || !hero || !before || !after || !player_y_q16 || m->room != UINT32_C(0x801961e8)) return 0;
    if (!bfm_rootrick_hero_step(hero, m->ordinary, m->ground, before, input, camera_yaw, field_tick,
                                can_climb, m->position, &m->player.root, &m->player.pose) ||
        !publish_root(m)) return 0;
    /* The camera and this field's drawing use the completed player model.
     * A later platform actor can carry his physical foot for the next step. */
    if (!bfm_rootrick_hero_carry_after(hero,m->ground,after,m->position)) return 0;
    m->airborne = !hero->grounded;
    m->box_air_action = hero->phase == BFM_ROOTRICK_HERO_JUMP ||
                        hero->phase == BFM_ROOTRICK_HERO_VAULT;
    /* The gripped and airborne actions own their local speed. Preserve the
     * completed action's momentum when a later script hands off to hurt. */
    if (m->box_air_action || hero->attached || hero->phase == BFM_ROOTRICK_HERO_LANDING) {
        m->speed = hero->forward_speed;
        m->fall_speed = hero->vertical_speed;
    }
    m->player.events = 0;
    *player_y_q16 = (int32_t)m->position[1];
    return 1;
}
