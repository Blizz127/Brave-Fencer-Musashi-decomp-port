/* SPDX-License-Identifier: MIT */
#include "bedroom_travel.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>

enum { DISPATCH, TURN, START_WALK, WALK, FINISH };
enum { Q = 65536 };
static const uint32_t scene_base = UINT32_C(0x80128158);
static const double angle_unit = 0.0015339807878856412; /* 2*pi / 4096 */

typedef struct {
    int n[3];
    int32_t d;
} Plane;

typedef struct {
    const uint8_t *data;
    size_t size;
    unsigned x, z, width, depth;
    size_t cells, distance, normal, indices, triangles, quads;
} Floor;

struct BfmBedroomTravel {
    BfmBedroomRoot root;
    Floor floor;
    int state, action, goal_x, goal_z, goal_yaw;
    int64_t position[3];
    int speed, fall_speed, airborne, landed;
    int retreat, saved_yaw, entry_action;
    unsigned entrance_ticks;
};

static unsigned u16(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static int s16(const uint8_t *p)
{
    unsigned v = u16(p);
    return v < 32768 ? (int)v : (int)v - 65536;
}

static uint32_t u32(const uint8_t *p)
{
    return u16(p) | (uint32_t)u16(p + 2) << 16;
}

static int fits(size_t bytes, size_t offset, size_t count)
{
    return offset <= bytes && count <= bytes - offset;
}

static const uint8_t *mounted(const uint8_t *s, size_t bytes, uint32_t address, size_t count)
{
    if (address < scene_base || !fits(bytes, address - scene_base, count)) return NULL;
    return s + (address - scene_base);
}

static int64_t floor_div(int64_t a, int64_t b)
{
    return a / b - (a % b < 0);
}

static int read_floor(Floor *f, const uint8_t *p, size_t size)
{
    f->data = p;
    f->size = size;
    f->x = u16(p);
    f->z = u16(p + 2);
    f->width = u16(p + 4);
    f->depth = u16(p + 6);
    f->cells = u32(p + 8);
    f->distance = u32(p + 12);
    f->normal = u32(p + 16);
    f->indices = u32(p + 20);
    f->triangles = u32(p + 24);
    f->quads = u32(p + 28);
    return f->width && f->depth && f->width <= 512 && f->depth <= 512 &&
        f->cells >= 32 && fits(f->distance, f->cells, 4u * f->width * f->depth) &&
        f->distance <= f->normal && f->normal <= f->indices &&
        f->indices <= f->triangles && f->triangles <= f->quads && f->quads <= size &&
        (f->normal - f->distance) % 4 == 0 && (f->indices - f->normal) % 8 == 0 &&
        (f->triangles - f->indices) % 2 == 0 && (f->quads - f->triangles) % 18 == 0;
}

static int read_planes(const Floor *f, unsigned id, Plane p[5], unsigned *flags)
{
    size_t begin = f->triangles, limit = f->quads;
    unsigned count = 4;
    if (id & 0x8000u) {
        id &= 0x7fffu;
        begin = f->quads;
        limit = f->size;
        count = 5;
    }
    size_t stride = 2 + 4 * count;
    if (id >= (limit - begin) / stride) return 0;
    const uint8_t *face = f->data + begin + id * stride;
    *flags = u16(face);
    for (unsigned i = 0; i < count; ++i) {
        unsigned ni = u16(face + 2 + 4*i), di = u16(face + 4 + 4*i);
        if (ni >= (f->indices - f->normal) / 8 ||
            di >= (f->normal - f->distance) / 4) return 0;
        const uint8_t *normal = f->data + f->normal + ni * 8;
        for (unsigned j = 0; j < 3; ++j) {
            p[i].n[j] = s16(normal + 2*j);
            if (abs(p[i].n[j]) > 4096) return 0;
        }
        uint32_t d = u32(f->data + f->distance + 4*di);
        p[i].d = (int32_t)((int64_t)d - (d > INT32_MAX ? INT64_C(4294967296) : 0));
    }
    return (int)count;
}

static int64_t side(const Plane *p, const int xyz[3])
{
    return p->d + (int64_t)p->n[0] * xyz[0] +
           (int64_t)p->n[1] * xyz[1] + (int64_t)p->n[2] * xyz[2];
}

static int cell_faces(const Floor *f, const int16_t start[3], const uint8_t **ids, unsigned *length)
{
    int x = (start[0] + 32768) / 128 - (int)f->x;
    int z = (start[2] + 32768) / 128 - (int)f->z;
    if (x < 0 || z < 0 || x >= (int)f->width || z >= (int)f->depth) return 0;
    const uint8_t *cell = f->data + f->cells + 4 * (x + z * f->width);
    unsigned first = u16(cell), count = u16(cell + 2);
    if ((first & 1) || !fits(f->triangles - f->indices, first, 2*count)) return -1;
    *ids = f->data + f->indices + first;
    *length = count;
    return 1;
}

/* The actor's shadow projects vertically, including while its feet are
 * airborne. Faces are stored bottom to top for this projection. */
static int project_shadow(BfmBedroomTravel *t)
{
    BfmBedroomRoot *r = &t->root;
    r->shadow_visible = 0;
    const uint8_t *ids;
    unsigned count;
    int valid = cell_faces(&t->floor, r->xyz, &ids, &count);
    if (valid != 1) return valid == 0;
    int ray[3] = {r->xyz[0], r->xyz[1] - 20, r->xyz[2]};
    while (count) {
        Plane planes[5];
        unsigned flags;
        int n = read_planes(&t->floor, u16(ids + 2*(--count)), planes, &flags);
        if (!n) return 0;
        int up = -planes[0].n[1];
        if (up <= 0 || (flags & 0x300u)) continue;
        int64_t start = side(planes, ray);
        if (start < 0 || start >= 768 * up) continue;
        int hit[3] = {ray[0], ray[1] + (int)(start/up), ray[2]};
        int j = 1;
        while (j < n && side(planes + j, hit) >= -12032) ++j;
        if (j != n || hit[1]-2 < INT16_MIN || hit[1]-2 > INT16_MAX) continue;
        hit[1] -= 2;
        for (j = 0; j < 3; ++j) {
            r->shadow_xyz[j] = (int16_t)hit[j];
            r->shadow_normal[j] = (int16_t)planes[0].n[j];
        }
        r->shadow_visible = 1;
        break;
    }
    return 1;
}

/* Follow sloping floor using a downward foot probe. In the air the player
 * makes contact only when the physical step crosses a room surface. */
static int contact(const Floor *f, const int16_t start[3], const int next[3],
                   int airborne, int end[3], int16_t normal[3])
{
    const uint8_t *ids;
    unsigned count;
    int valid = cell_faces(f, start, &ids, &count);
    if (valid != 1) return valid;
    int near[3] = {start[0], start[1], start[2]};
    int far[3] = {next[0], next[1], next[2]};
    if (!airborne) {
        int length = (int)hypot(next[0] - start[0], next[2] - start[2]);
        if (length < 3) length = 4;
        else if (length < 5) ++length;
        near[1] -= 4;
        far[1] += length + 1;
    }
    for (unsigned i = 0; i < count; ++i) {
        Plane planes[5];
        unsigned flags;
        int n = read_planes(f, u16(ids + 2*i), planes, &flags);
        if (!n) return -1;
        const int *axis = planes[0].n;
        int64_t from = side(planes, near), to = side(planes, far);
        if (axis[1] >= -1024 || from < 0 || to >= 0) continue;
        int cross[3];
        for (int j = 0; j < 3; ++j)
            cross[j] = near[j] + (int)((far[j] - near[j]) * from / (from - to));
        int j = 1;
        while (j < n && side(planes + j, cross) >= -12032) ++j;
        if (j != n) continue;
        int squared = axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2];
        int64_t projected[3];
        for (j = 0; j < 3; ++j) {
            int foot = far[j] + (int)(-to * axis[j] / squared);
            projected[j] = (int64_t)foot * Q + (axis[j] < 0 ? Q-1 : 0) + axis[j]*32;
        }
        for (int pass = 0; pass < 32; ++pass) {
            for (j = 0; j < 3; ++j) {
                projected[j] += axis[j]*16;
                end[j] = (int)floor_div(projected[j], Q);
            }
            if (side(planes, end) >= (axis[1] < -3584 ? 6144 : 12032)) {
                for (j = 0; j < 3; ++j) normal[j] = (int16_t)axis[j];
                return 1;
            }
        }
        return -1;
    }
    return 0;
}

BfmBedroomTravel *bfm_bedroom_travel_open(const uint8_t *scene, size_t bytes, uint32_t room_address, unsigned entrance)
{
    if (!scene) return NULL;
    const uint8_t *room = mounted(scene, bytes, room_address, 0xcc);
    if (!room) return NULL;
    if (entrance > 255) return NULL;
    const uint8_t *list = mounted(scene, bytes, u32(room + 0xc8), 4*(entrance+1));
    const uint8_t *grid = mounted(scene, bytes, u32(room + 0x88), 32);
    if (!list || !grid) return NULL;
    const uint8_t *entry = mounted(scene, bytes, u32(list+4*entrance), 24);
    if (!entry) return NULL;
    BfmBedroomTravel *t = calloc(1, sizeof *t);
    if (!t) return NULL;
    int spawn[3], foot[3];
    for (int j = 0; j < 3; ++j) {
        spawn[j] = s16(entry + 16 + 2*j);
        t->root.xyz[j] = (int16_t)spawn[j];
        t->position[j] = (int64_t)spawn[j] * Q;
    }
    t->root.yaw = u16(entry + 22) & 4095;
    if (!read_floor(&t->floor, grid, bytes - (size_t)(grid-scene)) ||
        contact(&t->floor, t->root.xyz, spawn, 0, foot, t->root.normal) != 1 ||
        !project_shadow(t)) {
        free(t);
        return NULL;
    }
    return t;
}

void bfm_bedroom_travel_free(BfmBedroomTravel *t)
{
    free(t);
}

const BfmBedroomRoot *bfm_bedroom_travel_root(const BfmBedroomTravel *t)
{
    return t ? &t->root : NULL;
}

static void begin(BfmBedroomTravel *t, int action)
{
    t->root.busy = 1;
    t->state = DISPATCH;
    t->action = action;
}

int bfm_bedroom_travel_turn(BfmBedroomTravel *t, unsigned yaw, int entry_action)
{
    if (!t || t->root.busy || yaw > 4095) return 0;
    t->goal_yaw = (int)yaw;
    begin(t, TURN);
    t->entry_action = entry_action;
    return 1;
}

int bfm_bedroom_travel_entrance(BfmBedroomTravel *t, unsigned updates)
{
    if (!t || !updates || updates > 255 || t->root.busy) return 0;
    t->entrance_ticks = updates;
    t->entry_action = 1;
    begin(t, START_WALK);
    return 1;
}

int bfm_bedroom_travel_walk(BfmBedroomTravel *t, int16_t x, int16_t z, int entry_action)
{
    if (!t || t->root.busy) return 0;
    t->entry_action = entry_action;
    t->goal_x = x;
    t->goal_z = z;
    begin(t, START_WALK);
    return 1;
}

/* The 10-bit slope selects an angular sample inside its interval. Computing
 * the sample directly keeps the trigonometry independent of a lookup bank. */
static int direction(int64_t x, int64_t z)
{
    uint64_t a = (uint64_t)llabs(x), b = (uint64_t)llabs(z);
    if (!a && !b) return 0;
    uint64_t ratio = a <= b ? a*1024/b : b*1024/a;
    int angle = (int)lround(atan((ratio + 0.375) / 1024.0) / angle_unit);
    if (a > b) angle = 1024 - angle;
    if (z < 0) angle = 2048 - angle;
    return (x < 0 ? -angle : angle) & 4095;
}

static int turn_toward(BfmBedroomTravel *t, int goal)
{
    int delta = goal - (int)t->root.yaw;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    int step = delta / 4;
    if (!step) step = delta / 2;
    if (!step) step = delta;
    t->root.yaw = (unsigned)((int)t->root.yaw + step) & 4095u;
    return delta;
}

void bfm_bedroom_travel_look(BfmBedroomTravel *t, int yaw)
{
    turn_toward(t, yaw);
}

int bfm_bedroom_travel_turn_to_point(BfmBedroomTravel *t, int x, int z)
{
    int dx = t->root.xyz[0] - x, dz = t->root.xyz[2] - z;
    int heading = (int)lround(atan2(dx, dz) / angle_unit) & 4095;
    return turn_toward(t, heading) == 0;
}

void bfm_bedroom_travel_face_point(BfmBedroomTravel *t, int x, int z)
{
    t->root.yaw = (unsigned)direction(t->root.xyz[0] - x, t->root.xyz[2] - z);
}

static int sine(int yaw)
{
    return (int)lround(4096 * sin((yaw & 4095) * angle_unit));
}

int bfm_bedroom_travel_retreat(BfmBedroomTravel *t, int16_t distance)
{
    /* This command walks a local Z offset while retaining the player's facing. */
    if (!t || t->root.busy) return 0;
    int yaw = (int)t->root.yaw;
    int x = t->root.xyz[0] + (int)floor_div((int64_t)distance * sine(yaw), 4096);
    int z = t->root.xyz[2] + (int)floor_div((int64_t)distance * sine(yaw + 1024), 4096);
    if (x < INT16_MIN || x > INT16_MAX || z < INT16_MIN || z > INT16_MAX ||
        !bfm_bedroom_travel_walk(t, (int16_t)x, (int16_t)z, 0)) return 0;
    t->retreat = 1;
    t->saved_yaw = yaw;
    return 1;
}

static int walk_step(BfmBedroomTravel *t, const int32_t eye[3], const int32_t target[3])
{
    int dx = t->goal_x - t->root.xyz[0], dz = t->goal_z - t->root.xyz[2];
    if (!t->entrance_ticks && (int64_t)dx*dx + (int64_t)dz*dz < 16*16) {
        t->root.xyz[0] = (int16_t)t->goal_x;
        t->root.xyz[2] = (int16_t)t->goal_z;
        for (int j = 0; j < 3; ++j)
            t->position[j] = (int64_t)t->root.xyz[j]*Q + (t->position[j] & (Q-1));
        if (t->entry_action || t->retreat) t->state = FINISH;
        else t->root.busy = 0;
        t->root.locomotion_edge = BFM_TRAVEL_STAND;
        return project_shadow(t);
    }
    if (!eye || !target) return 0;
    int camera = (direction((int64_t)target[0]-eye[0], (int64_t)target[2]-eye[2]) + 2048) & 4095;
    /* A doorway synthesizes its next input from the actor's current heading.
     * Quantizing that input can change the heading again on the next tick. */
    int facing = t->entrance_ticks ? (int)t->root.yaw : direction(dx, dz) + 2048;
    int relative = (facing - camera) & 4088;
    int pad_x = sine(relative) / 32, pad_z = sine(relative + 1024) / 32;
    if (pad_x > 127) pad_x = 127;
    if (pad_z < -127) pad_z = -127;
    t->root.yaw = (unsigned)(camera + direction(pad_x, pad_z)) & 4095u;
    if (t->state == START_WALK || t->landed) {
        t->state = WALK;
        t->landed = 0;
        t->root.locomotion_edge = BFM_TRAVEL_WALK;
        return 1;
    }
    int axis_x = sine((int)t->root.yaw), axis_z = sine((int)t->root.yaw + 1024);
    if (axis_x*axis_x + axis_z*axis_z < 4096*4096) {
        axis_x = (int)floor_div(axis_x*4104, 4096);
        axis_z = (int)floor_div(axis_z*4104, 4096);
    }
    int magnitude = (int)hypot(pad_x, pad_z);
    if (magnitude > 127) magnitude = 127;
    if (t->airborne) {
        t->speed += Q/16;
        if (t->speed > 11*Q + 5*Q/8) t->speed = 11*Q + 5*Q/8;
        t->fall_speed += Q + 5*Q/8;
    } else {
        t->speed = 11*Q;
        t->fall_speed = 0;
    }
    int speed = t->speed * magnitude / 127;
    int64_t update[3] = {
        t->position[0] + floor_div(-(int64_t)speed*axis_x, 4096),
        t->position[1] + t->fall_speed,
        t->position[2] + floor_div(-(int64_t)speed*axis_z, 4096)
    };
    int next[3], end[3];
    for (int j = 0; j < 3; ++j) {
        int64_t high = floor_div(update[j], Q);
        if (high < INT16_MIN || high > INT16_MAX) return 0;
        next[j] = (int)high;
    }
    int hit = contact(&t->floor, t->root.xyz, next, t->airborne, end, t->root.normal);
    if (hit < 0) return 0;
    t->landed = t->airborne && hit;
    if (t->landed) t->root.locomotion_edge = BFM_TRAVEL_LAND;
    else if (!hit && !t->airborne) t->root.locomotion_edge = BFM_TRAVEL_FALL;
    t->airborne = !hit;
    if (!hit) {
        t->root.normal[0] = t->root.normal[2] = 0;
        t->root.normal[1] = -4096;
    }
    for (int j = 0; j < 3; ++j) {
        int high = hit ? end[j] : next[j];
        if (high < INT16_MIN || high > INT16_MAX) return 0;
        t->root.xyz[j] = (int16_t)high;
        t->position[j] = (int64_t)high * Q + (update[j] & (Q-1));
    }
    if (t->entrance_ticks && !--t->entrance_ticks) {
        t->state = FINISH;
        t->root.locomotion_edge = BFM_TRAVEL_STAND;
    }
    return project_shadow(t);
}

int bfm_bedroom_travel_step(BfmBedroomTravel *t,
                            const int32_t eye[3], const int32_t target[3])
{
    if (!t) return 0;
    t->root.locomotion_edge = BFM_TRAVEL_NONE;
    if (!t->root.busy) return 1;
    switch (t->state) {
    case DISPATCH:
        t->state = t->action;
        return 1;
    case TURN: {
        if (!turn_toward(t, t->goal_yaw)) {
            if (t->entry_action) t->state = FINISH;
            else t->root.busy = 0;
        }
        return 1;
    }
    case START_WALK:
    case WALK: {
        int ok = walk_step(t, eye, target);
        if (t->retreat) t->root.yaw = (unsigned)t->saved_yaw;
        return ok;
    }
    case FINISH:
        t->retreat = 0;
        t->root.busy = 0;
        return 1;
    }
    return 0;
}
