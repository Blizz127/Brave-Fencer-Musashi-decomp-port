/* SPDX-License-Identifier: MIT */
#include "village_arrival_motion.h"
#include "forest_motion.h"
#include "village_ground.h"
#include "village_residents.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { Q = 65536, GRAVITY = 13*Q/8 };
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
    unsigned brake_count, brake_frame, brake, ingress, ingress_walk;
    int airborne, landed;
    int32_t fall_speed;
    uint16_t material;
    BfmBedroomRoot crossing;
    int64_t position[3];
    int32_t starting_speed, speed;
    int walking, pending, message_due;
    enum { NO_TALK, TALK_APPROACH, TALK_FACE, TALK_HOLD, READ_SIGN } talk;
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
        return 1;
    }
    return bfm_bedroom_travel_air_shadow(m->scene.data,m->scene.bytes,m->room,r);
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
    const uint8_t *bank = at(&m->resident,read32(room+0x4c),6*4);
    if (!m->ground || !m->ordinary || !bank) return 0;
    uint32_t address = read32(bank+5*4);
    const uint8_t *brake = at(&m->resident,address,12);
    if (!brake || read32(brake) != 4 || !at(&m->resident,read32(brake+4),4) ||
        read32(brake+8) != 19 || address > UINT32_MAX-12-64*4) return 0;
    m->brake_keys = brake+12;
    for (unsigned i = 0; i < 64; ++i) {
        const uint8_t *key = at(&m->resident,address+12+4*i,4);
        if (!key) return 0;
        if (read32(key) == 1) {
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

static int move(BfmVillageMotion *m, int speed, int vertical, int require_floor)
{
    ForestPlayerState velocity = {0};
    velocity.field_2c.word = -speed;

    if (!bfm_forest_level_velocity(&velocity, 0, (int16_t)m->player.root.yaw, 0)) return 0;
    /* Gravity can pass the horizontal speed bound on a long fall. */
    velocity.velocity[1] = (uint32_t)vertical;
    int64_t next[3];
    BfmBedroomRoot candidate = m->player.root;
    for (int i = 0; i < 3; ++i) {
        uint32_t v = velocity.velocity[i];
        int64_t signed_v = (int64_t)v - (v > INT32_MAX ? INT64_C(4294967296) : 0);
        next[i] = m->position[i] + signed_v;
        int64_t whole = high(next[i]);
        if (whole < INT16_MIN || whole > INT16_MAX) return require_floor;
        candidate.xyz[i] = (int16_t)whole;
    }
    if (require_floor) {
        BfmForestContact floor;
        BfmVillageGroundResult hit = m->airborne ?
            bfm_village_ground_air(m->ground,m->player.root.xyz,candidate.xyz,&floor) :
            bfm_village_ground_wall_step(m->ground,m->player.root.xyz,candidate.xyz,&floor,
                                         &m->player.wall_contact);
        if (hit == BFM_VILLAGE_GROUND_BLOCKED) return 1;
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

static int heading(int64_t x, int64_t z)
{
    int64_t a = llabs(x), b = llabs(z);
    if (!a && !b) return 0;
    int ratio = (int)(a <= b ? a*1024/b : b*1024/a);
    int angle = (int)lround(atan((ratio+0.375)/1024.0)/angle_unit);
    if (a > b) angle = 1024-angle;
    if (z < 0) angle = 2048-angle;
    return (x < 0 ? -angle : angle) & 4095;
}

int bfm_village_motion_view_yaw(const int32_t eye[3], const int32_t target[3])
{
    if (!eye || !target) return 0;
    return heading((int64_t)target[0]-eye[0], (int64_t)target[2]-eye[2]);
}

static int turn(BfmVillageMotion *m)
{
    int delta = (int)((m->turn-m->player.root.yaw+2048) & 4095)-2048;
    int step = delta/4;
    if (!step) step = delta/2;
    if (!step) step = delta;
    m->player.root.yaw = (m->player.root.yaw + (unsigned)step) & 4095;
    return delta == 0;
}

static int pad_axis(int angle, int power)
{
    int value = (int)lround(4096*sin((angle & 4095)*angle_unit))/32;
    if (power) value = value*power/256;
    return value < -127 ? -127 : value > 127 ? 127 : value;
}

/* The input service measures a single axis directly. For diagonals it
 * quantizes the squared length to a base-four mantissa before taking its root. */
static int pad_magnitude(int x, int z)
{
    if (!x || !z) return abs(x) + abs(z);
    unsigned squared = (unsigned)(x*x + z*z), shift = 0;
    while ((squared >> shift) > 255) shift += 2;
    return (int)sqrt((double)((squared >> shift) << shift));
}

static int walk_toward(BfmVillageMotion *m, int dx, int dz, int power, int camera,
                       BfmBedroomAction *action)
{
    int relative = (heading(dx,dz)-camera) & 4088;
    int px = pad_axis(relative,power), pz = pad_axis(relative+1024,power);
    m->player.root.yaw = (unsigned)(camera+2048+heading(px,pz)) & 4095;
    /* Keeping WALK selected preserves the walking cycle at partial deflection. */
    if (power || !m->walking) *action = BFM_BEDROOM_WALK;
    if (!m->walking) {
        m->walking = 1;
        return 1;
    }
    int magnitude = pad_magnitude(px,pz);
    if (magnitude > 127) magnitude = 127;
    return move(m, 11*Q*magnitude/127, 0, 1);
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
        m->turn = (unsigned)heading(m->talk_x-m->resident_x,m->talk_z-m->resident_z);
        m->talk = TALK_FACE;
    } else if (m->talk == TALK_FACE && turn(m)) {
        m->talk = TALK_HOLD;
    }
    return 1;
}

static int ordinary_input(BfmVillageMotion *m, int x, int y, int camera, BfmBedroomAction *action)
{
    int magnitude = pad_magnitude(x,y);
    int steer = magnitude >= 16;
    if (magnitude > 127) magnitude = 127;
    if (steer) m->player.root.yaw = (unsigned)(camera+heading(-x,y)) & 4095;
    if (m->landed) {
        m->landed = 0;
        m->walking = steer;
        m->speed = ((m->speed+Q-1)/Q)*Q;
        *action = steer ? BFM_BEDROOM_WALK : BFM_BEDROOM_STAND;
        return 1;
    }
    if (!steer && !m->airborne) {
        if (m->walking) {
            *action = BFM_BEDROOM_STAND;
            m->brake = m->speed == 11*Q;
            m->brake_frame = 0;
            m->walking = 0;
        } else if (m->brake) {
            if (++m->brake_frame < m->brake_count)
                return move(m,4*Q,0,1);
            m->brake = 0;
        }
        return 1;
    }
    m->brake = 0;
    if (!m->walking && !m->airborne) {
        m->walking = 1;
        m->speed = m->starting_speed;
        *action = BFM_BEDROOM_WALK;
        return 1;
    }
    int was_airborne = m->airborne;
    int speed = m->speed+Q/16;
    int limit = m->airborne ? 11*Q+5*Q/8 : 11*Q;
    m->speed = speed < limit ? speed : limit;
    if (steer) speed = speed*magnitude/127;
    if (!m->airborne && speed < 9*Q+Q/4) *action = BFM_BEDROOM_WALK;
    if (m->airborne && m->fall_speed < 31*Q) m->fall_speed += GRAVITY;
    if (!move(m,speed,m->airborne ? m->fall_speed : 0,1)) return 0;
    if (m->airborne && !was_airborne) *action = BFM_BEDROOM_FALL;
    if (m->landed) *action = BFM_BEDROOM_LAND;
    return 1;
}

static const uint8_t *exit_floor(const BfmVillageMotion *m)
{
    if (m->material == 0x106) return m->cleft;
    if (m->material == (m->room == 0x80192b18u ? 0x105 : 5)) return m->outgoing;
    return NULL;
}

static int advance(BfmVillageMotion *m, int stick_x, int stick_y, int camera)
{
    BfmVillagePlayer *p = &m->player;
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    p->events = 0;
    p->wall_contact = 0;
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
        /* The lower ridge support is marked 0x105; the returning side
         * of the same seam uses the upper field's ordinary floor tag 5. */
        const uint8_t *door = exit_floor(m);
        if (!m->transfer && door) {
            m->transfer = door;
            m->crossing = p->root;
            p->destination = (uint16_t)read16(m->transfer);
            p->entrance = m->transfer[2];
            p->events |= BFM_VILLAGE_TRANSFER;
        }
        if (m->ingress) {
            if (m->ingress == 1) {
                action = BFM_BEDROOM_WALK;
            } else if (m->ingress <= 1 + m->ingress_walk) {
                /* The room entry synthesizes a direction from the actor's
                 * own heading in the current view, at full walking speed. */
                int facing = (int)p->root.yaw;
                int relative = (facing - camera - 2048) & 4088;
                int px = pad_axis(relative,0), pz = pad_axis(relative+1024,0);
                p->root.yaw = (unsigned)(camera+2048+heading(px,pz)) & 4095;
                int power = pad_magnitude(px,pz);
                if (!move(m,11*Q*(power > 127 ? 127 : power)/127,0,1)) return 0;
            } else {
                if (!m->brake) {
                    action = BFM_BEDROOM_STAND;
                    /* The incoming release reaches the braking animation
                     * with eleven four-unit strides left. */
                    m->brake = 1;
                    m->brake_frame = m->brake_count - 12;
                }
                if (!ordinary_input(m,0,0,camera,&action)) return 0;
            }
            if (m->ingress == 6 + m->ingress_walk) {
                p->controlled = 1;
                m->ingress = 0;
            } else ++m->ingress;
        } else if (m->talk != NO_TALK) {
            if (!talking_input(m,camera,&action)) return 0;
        } else if (!ordinary_input(m,stick_x,stick_y,camera,&action)) return 0;
        break;
    }
    }
    if (!publish_root(m)) return 0;
    if (p->phase <= BFM_VILLAGE_IMPACT) return 1;
    if (!bfm_bedroom_motion_step(m->ordinary,action,&p->pose)) return 0;
    if (m->brake) {
        p->pose.pose = at(&m->resident,read32(m->brake_keys+4*m->brake_frame),256);
        p->pose.show_lumina = 0;
    }
    return 1;
}

int bfm_village_motion_step(BfmVillageMotion *m, int x, int y, int camera_yaw)
{
    if (!m || x < -127 || x > 127 || y < -127 || y > 127) return 0;
    BfmVillageMotion next = *m;
    if (!advance(&next,x,y,camera_yaw & 4095)) return 0;
    *m = next;
    return 1;
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

int bfm_village_motion_staged(BfmVillageMotion *m, const BfmBedroomRoot *root)
{
    return bfm_village_motion_porch(m, root, 0);
}

int bfm_village_motion_porch(BfmVillageMotion *m, const BfmBedroomRoot *root, int knock)
{
    if (!m || !root || m->player.phase != BFM_VILLAGE_CONTROL || m->talk != NO_TALK ||
        root->yaw >= 4096) return 0;
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    switch (root->locomotion_edge) {
    case BFM_TRAVEL_WALK: action = BFM_BEDROOM_WALK; break;
    case BFM_TRAVEL_FALL: action = BFM_BEDROOM_FALL; break;
    case BFM_TRAVEL_LAND: action = BFM_BEDROOM_LAND; break;
    case BFM_TRAVEL_STAND: action = BFM_BEDROOM_STAND; break;
    case BFM_TRAVEL_NONE: break;
    default: return 0;
    }
    if (knock) action = BFM_BEDROOM_KNOCK;
    BfmBedroomRoot next = *root;
    if (!shadow(m,&next) || !bfm_bedroom_motion_step(m->ordinary,action,&m->player.pose))
        return 0;
    m->player.root = next;
    m->player.events = 0;
    return 1;
}

int bfm_village_motion_place(BfmVillageMotion *m, const BfmBedroomRoot *root)
{
    if (!m || !root || !m->player.controlled || m->talk != NO_TALK || root->yaw >= 4096)
        return 0;
    BfmVillageMotion next = *m;
    for (unsigned i = 0; i < 3; ++i)
        next.position[i] = (int64_t)root->xyz[i]*Q;
    next.player.root = *root;
    next.player.root.busy = 0;
    next.player.root.locomotion_edge = BFM_TRAVEL_NONE;
    next.airborne = next.landed = next.brake = next.walking = 0;
    next.speed = next.fall_speed = 0;
    next.material = 0;
    if (!publish_root(&next) ||
        !bfm_bedroom_motion_step(next.ordinary,BFM_BEDROOM_ENTER,&next.player.pose)) return 0;
    *m = next;
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
        if (!door || read16(door) != (upper ? 0x1011 : 0x1010) ||
            entrance != 1 || door[2] != entrance) return NULL;
    } else if (!main || upper || entrance != 10) return NULL;
    BfmVillageMotion *m = calloc(1,sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene,bytes,scene_base};
    m->resident = departed ? departed->resident : (Region){main, main_bytes, main_base};
    m->room = room_address;
    const uint8_t *header = at(&m->scene,upper ? 0x801854b0u : 0x8018d260u,4);
    const uint8_t *room = at(&m->scene,room_address,0xcc);
    if (!header || read32(header) != room_address || !room || !load_field(m,room)) goto fail;
    const uint8_t *entries = at(&m->scene,read32(room+0xc8),(entrance+1)*4);
    const uint8_t *entry = entries ? at(&m->scene,read32(entries+4*entrance),36) : NULL;
    if (upper) {
        const uint8_t *cleft = at(&m->scene,read32(room+0xc8)+8,4);
        m->cleft = cleft ? at(&m->scene,read32(cleft),36) : NULL;
        if (!m->cleft || read32(m->cleft) != 0x02003025u) goto fail;
    }
    m->outgoing = entries ? at(&m->scene,read32(entries+4),36) : NULL;
    if (!m->outgoing || read16(m->outgoing) != (upper ? 0x1010 : 0x1011) ||
        m->outgoing[2] != 1 || !entry ||
        (!departed && (read16(entry) != 0x2017 || entry[2]))) goto fail;
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    const uint8_t *velocity = at(&m->scene,upper ? 0x80187b0cu : 0x8018fb1cu,12);
    if (!velocity || read32(velocity+8) < 0u-11u*Q || !read32(velocity+8)) goto fail;
    m->starting_speed = (int32_t)(0u-read32(velocity+8));
    BfmBedroomRoot root = {0};
    if (departed) root = departed->crossing;
    else {
        for (int i = 0; i < 3; ++i) root.xyz[i] = (int16_t)read16(entry+16+2*i);
        root.yaw = (unsigned)read16(entry+22) & 4095;
        root.normal[1] = -4096;
    }
    if (bfm_village_motion_place(m,&root)) return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

BfmVillageMotion *bfm_village_motion_woods(const uint8_t *scene, size_t bytes,
                                           const uint8_t *main, size_t main_bytes)
{
    BfmVillageMotion *m = calloc(1,sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene,bytes,scene_base};
    m->resident = (Region){main,main_bytes,main_base};
    const uint8_t *header = at(&m->scene,0x801877c4u,4);
    if (!header || read32(header) != 0x8018ea38u) goto fail;
    m->room = read32(header);
    const uint8_t *room = at(&m->scene,m->room,0xcc);
    if (!room || !load_field(m,room)) goto fail;
    const uint8_t *entries = at(&m->scene,read32(room+0xc8),4);
    const uint8_t *entry = entries ? at(&m->scene,read32(entries),36) : NULL;
    const uint8_t *velocity = at(&m->scene,0x80189e20u,12);
    if (!entry || read32(entry) != 0x00021011u || !velocity ||
        read32(velocity+8) < 0u-11u*Q || !read32(velocity+8)) goto fail;
    m->starting_speed = (int32_t)(0u-read32(velocity+8));
    BfmBedroomRoot root = {0};
    for (int i = 0; i < 3; ++i) root.xyz[i] = (int16_t)read16(entry+16+2*i);
    root.yaw = (unsigned)read16(entry+22) & 4095;
    root.normal[1] = -4096;
    m->player.phase = BFM_VILLAGE_CONTROL;
    m->player.controlled = 1;
    if (!bfm_village_motion_place(m,&root)) goto fail;
    const uint8_t *walk = at(&m->scene,0x8018b5b8u,12);
    if (!walk || read32(walk) != 0x8016f9c4u || read32(walk+8) != 0x8016f494u ||
        !read32(walk+4) || read32(walk+4) > 255 || m->brake_count < 12) goto fail;
    m->ingress_walk = read32(walk+4);
    m->ingress = 1;
    m->player.controlled = 0;
    return m;
fail:
    bfm_village_motion_free(m);
    return NULL;
}

BfmVillageMotion *bfm_village_motion_arrive(const BfmVillageMotion *departed, const uint8_t *scene,
                                            size_t bytes, uint32_t room_address)
{
    return walking_field(departed, NULL, 0, scene, bytes, room_address, 1);
}

BfmVillageMotion *bfm_village_motion_resume(const uint8_t *scene, size_t bytes,
                                            uint32_t room_address,
                                            const uint8_t *main, size_t main_bytes)
{
    return walking_field(NULL, main, main_bytes, scene, bytes, room_address, 10);
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
    m->resident_x = resident[0];
    m->resident_z = resident[2];
    m->talk_x = (int16_t)x;
    m->talk_z = (int16_t)z;
    m->walking = 0;
    m->talk_dispatch = 2;
    m->talk = TALK_APPROACH;
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
    m->turn = (unsigned)heading(m->player.root.xyz[0]-point[0],m->player.root.xyz[2]-point[2]);
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
    return m && bfm_bedroom_orb_palette(m->ordinary, 0x80190cd4u, orb, tiles);
}

void bfm_village_motion_free(BfmVillageMotion *m)
{
    if (m) {
        bfm_bedroom_motion_free(m->ordinary);
        bfm_village_ground_free(m->ground);
    }
    free(m);
}
