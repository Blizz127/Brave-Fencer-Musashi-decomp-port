/* SPDX-License-Identifier: MIT */
#include "bedroom_visitors.h"
#include <math.h>
#include <stdlib.h>

static const uint32_t scene_base = UINT32_C(0x80128158);

typedef struct {
    uint32_t walk, idle, route, speech_table;
    unsigned parts;
    int entrance_z;
} VisitorClass;

/* The room's two constructors assign their own walk and idle animations
 * and separate movement tracks. All poses and movement records are mounted. */
static const VisitorClass classes[] = {
    {0x801bc4a0, 0x801bc508, 0x801bc6c8, 0x801bc78c, 7, -320},
    {0x801bc578, 0x801bc5e0, 0x801bc718, 0x801bc798, 10, -384}
};

typedef enum { ABSENT, WAITING, FOLLOWING, ARRIVED } VisitorState;
typedef struct {
    BfmBedroomVisitorPose frame;
    VisitorState state;
    uint32_t route_cursor, animation, cursor;
    unsigned hold, ticks, speech_delay;
    int speech_requested;
    int64_t x, z, vx, vz;
} Visitor;

struct BfmBedroomVisitors {
    const uint8_t *scene;
    size_t bytes;
    Visitor people[2];
};

static const uint8_t *span(const BfmBedroomVisitors *v, uint32_t address, size_t size)
{
    if (address < scene_base) return NULL;
    size_t offset = address - scene_base;
    if (offset > v->bytes || size > v->bytes - offset) return NULL;
    return v->scene + offset;
}

static unsigned half(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static uint32_t word(const uint8_t *p) { return half(p) | (uint32_t)half(p+2) << 16; }

static int set_frame(const BfmBedroomVisitors *v, Visitor *p, const VisitorClass *cl)
{
    const uint8_t *entry = span(v, p->cursor, 8);
    if (!entry) return 0;
    int duration = (int16_t)half(entry + 4);
    if (duration == -2) { /* Hold the final pose. */
        p->hold = 0;
        return 1;
    }
    if (duration == -1) { /* Restart at the first pose. */
        p->cursor = p->animation;
        entry = span(v, p->cursor, 8);
        if (!entry) return 0;
        duration = (int16_t)half(entry + 4);
    }
    uint32_t pose = word(entry);
    if (duration <= 0 || pose >> 24 != 0x81) return 0;
    pose &= ~UINT32_C(0x01000000);
    if (!span(v, pose, cl->parts * 8u)) return 0;
    p->frame.pose_address = pose;
    p->hold = (unsigned)duration;
    return 1;
}

static int animate(const BfmBedroomVisitors *v, Visitor *p, const VisitorClass *cl,
                   uint32_t animation)
{
    p->animation = p->cursor = animation;
    return set_frame(v, p, cl);
}

/* The matrix axes reflect each sampled quadrant within its 1,024 entries. */
static int axis(unsigned heading)
{
    unsigned phase = heading & 1023u;
    if (heading & 1024u) phase = 1023u - phase;
    int value = (int)lround(4096 * sin(phase * (6.28318530717958647693 / 4096)));
    return (heading & 2048u) ? -value : value;
}

static int travel(const BfmBedroomVisitors *v, Visitor *p, const VisitorClass *cl)
{
    if (!p->ticks) {
        const uint8_t *command = span(v, p->route_cursor, 2);
        if (!command) return 0;
        int duration = (int16_t)half(command);
        if (!duration) {
            p->state = ARRIVED;
            return animate(v, p, cl, cl->idle);
        }
        command = span(v, p->route_cursor, 6);
        if (!command || duration < 0 || p->route_cursor > UINT32_MAX-6) return 0;
        p->route_cursor += 6;
        p->ticks = (unsigned)duration;
        int yaw = (int)half(command+2) & 4095;
        int speed = (int16_t)half(command+4);
        p->vx = (int64_t)axis((unsigned)yaw) * speed * 16;
        p->vz = (int64_t)axis((unsigned)yaw+1024u) * speed * 16;
        p->frame.heading = yaw;
    }
    p->x += p->vx;
    p->z += p->vz;
    if (p->x < INT32_MIN || p->x > INT32_MAX || p->z < INT32_MIN || p->z > INT32_MAX)
        return 0;
    /* Signed high halves are the model's submitted world coordinates. */
    p->frame.position[0] = (int16_t)((uint32_t)p->x >> 16);
    p->frame.position[2] = (int16_t)((uint32_t)p->z >> 16);
    --p->ticks;
    return 1;
}

static int step(const BfmBedroomVisitors *v, Visitor *p, const VisitorClass *cl)
{
    /* A newly constructed model receives its first pose immediately, after
     * this update's animation pass. Its actor callback still runs below. */
    int created_this_update = p->state == WAITING && p->ticks != 0;
    if (!created_this_update && p->hold && --p->hold == 0) {
        if (p->cursor > UINT32_MAX-8) return 0;
        p->cursor += 8;
        if (!set_frame(v, p, cl)) return 0;
    }
    switch (p->state) {
    case ABSENT: return 1;
    case WAITING:
        if (p->ticks) --p->ticks;
        else {
            p->state = FOLLOWING;
            p->route_cursor = cl->route;
        }
        return 1;
    case FOLLOWING:
        return travel(v, p, cl);
    case ARRIVED:
        if (p->speech_requested) {
            if (!p->speech_delay) p->speech_delay = 8;
            else if (!--p->speech_delay) {
                const uint8_t *entry = span(v, cl->speech_table + 4, 4);
                if (!entry) return 0;
                p->speech_requested = 0;
                return animate(v, p, cl, word(entry));
            }
        }
        return 1;
    }
    return 0;
}

BfmBedroomVisitors *bfm_bedroom_visitors_open(const uint8_t *scene, size_t bytes)
{
    if (!scene) return NULL;
    BfmBedroomVisitors *v = calloc(1, sizeof *v);
    if (!v) return NULL;
    v->scene = scene;
    v->bytes = bytes;
    return v;
}

int bfm_bedroom_visitors_spawn(BfmBedroomVisitors *v)
{
    if (!v || v->people[0].state != ABSENT) return 0;
    BfmBedroomVisitors next = *v;
    for (unsigned i = 0; i < 2; ++i) {
        const VisitorClass *cl = &classes[i];
        Visitor *p = &next.people[i];
        if (!span(v, cl->route, 6) || !span(v, cl->idle, 8) ||
            !animate(v, p, cl, cl->walk)) return 0;
        p->state = WAITING;
        p->frame.visible = 1;
        p->ticks = 1;
        p->frame.position[1] = -130;
        p->frame.position[2] = cl->entrance_z;
        p->z = (int64_t)cl->entrance_z * 65536;
    }
    *v = next;
    return 1;
}

void bfm_bedroom_visitors_speak(BfmBedroomVisitors *v)
{
    if (v && v->people[0].state != ABSENT) v->people[0].speech_requested = 1;
}

int bfm_bedroom_visitors_step(BfmBedroomVisitors *v)
{
    if (!v) return 0;
    BfmBedroomVisitors next = *v;
    for (unsigned i=0; i<2; ++i)
        if (!step(&next, &next.people[i], &classes[i])) return 0;
    *v = next;
    return 1;
}

const BfmBedroomVisitorPose *bfm_bedroom_visitors_pose(const BfmBedroomVisitors *v,
                                                       unsigned visitor)
{
    return v && visitor < 2 ? &v->people[visitor].frame : NULL;
}

void bfm_bedroom_visitors_free(BfmBedroomVisitors *v) { free(v); }
