/* SPDX-License-Identifier: MIT */
#include "village_mayor_cast.h"
#include "bedroom_visitor.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { PARTS = 10, MAX_KEYS = 32 };

typedef struct {
    unsigned count;
    uint32_t poses[MAX_KEYS];
} Clip;

typedef struct {
    unsigned ticks;
    int dx, dz;
} Walk;

struct BfmVillageMayorCast {
    BfmBedroomVisitor *body;
    Clip walking, speaking;
    Walk outward, inward;
    int16_t home[3], position[3], porch[3];
    unsigned facing, yaw, turn_to, turn_ticks, depart, emerge;
    uint32_t pose;
    int visible;
};

static const uint8_t *span(const uint8_t *data, size_t size, uint32_t base,
                           uint32_t address, size_t length)
{
    if (!data || address < base || length > size || address-base > size-length) return NULL;
    return data + (address-base);
}

static const uint8_t *field(const uint8_t *scene, size_t bytes, uint32_t addr, size_t n)
{
    return span(scene, bytes, UINT32_C(0x80128158), addr, n);
}

static int load_clip(Clip *clip, const uint8_t *scene, size_t bytes, uint32_t address)
{
    for (unsigned i = 0; i <= MAX_KEYS; ++i) {
        const uint8_t *key = field(scene, bytes, address, 8);
        if (!key) return 0;
        uint32_t tagged = u32le(key), duration = u32le(key + 4);
        if (!tagged && duration == 0xffff) {
            clip->count = i;
            return i != 0;
        }
        uint32_t pose = tagged & ~UINT32_C(0x01000000);
        if (i == MAX_KEYS || tagged >> 24 != 0x81 || duration != 1 ||
            !field(scene, bytes, pose, PARTS * 8)) return 0;
        clip->poses[i] = pose;
        address += 8;
    }
    return 0;
}

static unsigned heading_at(const BfmVillageMayorCast *cast, unsigned ticks)
{
    int yaw = (int)cast->facing;
    int distance = (int)((cast->turn_to - cast->facing) & 4095u);
    if (distance > 2048) distance -= 4096;
    int target = yaw + distance;
    if (ticks > cast->turn_ticks) ticks = cast->turn_ticks;
    for (unsigned i = 0; i < ticks; ++i) yaw += (target - yaw) / 6;
    return (unsigned)yaw & 4095u;
}

/* Speed is Q8. The resident submits whole world units on each walking update. */
static int world_step(int axis, int speed)
{
    int product = axis * speed;
    return product / 1048576 - (product % 1048576 < 0);
}

static int leg(Walk *walk, const uint8_t *cue, unsigned yaw, const int16_t start[3])
{
    if (u32le(cue) != 0x27) return 0;
    int speed = s16le(cue + 4);
    walk->ticks = u16le(cue + 6);
    if (!walk->ticks || walk->ticks > 1024) return 0;
    BfmForestTransform root;
    bfm_forest_actor_root(yaw, start, &root);
    walk->dx = world_step(root.rotation[0][2], speed);
    walk->dz = world_step(root.rotation[2][2], speed);
    int x = start[0] + walk->dx * (int)walk->ticks;
    int z = start[2] + walk->dz * (int)walk->ticks;
    return x >= INT16_MIN && x <= INT16_MAX && z >= INT16_MIN && z <= INT16_MAX;
}

static void walk_to(int16_t result[3], const int16_t from[3], const Walk *walk, unsigned tick)
{
    result[0] = (int16_t)(from[0] + walk->dx * (int)tick);
    result[1] = from[1];
    result[2] = (int16_t)(from[2] + walk->dz * (int)tick);
}

BfmVillageMayorCast *bfm_village_mayor_cast_load(const uint8_t *scene, size_t bytes,
    uint32_t room, const uint8_t *mayor, size_t mayor_bytes, uint32_t shadow)
{
    const uint32_t event_base = UINT32_C(0x801ef468);
    const uint8_t *kit = span(mayor, mayor_bytes, event_base, 0x801f0900u, 32);
    const uint8_t *entry_cue = span(mayor, mayor_bytes, event_base, 0x801f0944u, 8);
    const uint8_t *exit_cue = span(mayor, mayor_bytes, event_base, 0x801f09c4u, 8);
    if (room != 0x80192b18u || !kit || !entry_cue || !exit_cue ||
        u32le(entry_cue) != 0x35 || u32le(exit_cue) != 0x35) return NULL;
    const uint8_t *enter = field(scene, bytes, u32le(entry_cue+4), 96);
    const uint8_t *leave = field(scene, bytes, u32le(exit_cue+4), 40);
    if (!enter || !leave || u32le(enter+40) != 15 || u32le(enter+56) != 15 ||
        u32le(leave+8) != 0x25 || u32le(leave+16) != 15 ||
        u32le(enter+44) > 4095 || u32le(enter+60) > 4095 ||
        u32le(leave+20) > 256) return NULL;
    BfmVillageMayorCast *cast = calloc(1, sizeof(*cast));
    if (!cast) return NULL;
    for (unsigned i = 0; i < 3; ++i) cast->home[i] = s16le(kit + i * 2);
    cast->facing = u16le(kit+6) & 4095u;
    cast->turn_to = u16le(leave+12) & 4095u;
    cast->turn_ticks = u32le(leave+20);
    /* At the door the knock and actor scripts start on successive turns.
     * After the goodbye, the resident selects his walking animation before
     * starting the timed turn back towards his house. */
    cast->emerge = 18 + u32le(enter+44) + u32le(enter+60);
    cast->depart = 3;
    uint32_t model = u32le(kit+16);
    if (!load_clip(&cast->walking, scene, bytes, u32le(kit+24)) ||
        !load_clip(&cast->speaking, scene, bytes, u32le(kit+28)) ||
        !leg(&cast->outward, enter+80, cast->facing, cast->home)) goto fail;
    walk_to(cast->porch, cast->home, &cast->outward, cast->outward.ticks);
    if (!leg(&cast->inward, leave+24, heading_at(cast, cast->turn_ticks), cast->porch)) goto fail;
    cast->body = bfm_bedroom_visitor_load(scene,bytes,&model,1,shadow);
    if (cast->body) return cast;
fail:
    free(cast);
    return NULL;
}

void bfm_village_mayor_cast_free(BfmVillageMayorCast *cast)
{
    if (!cast) return;
    bfm_bedroom_visitor_free(cast->body);
    free(cast);
}

void bfm_village_mayor_cast_step(BfmVillageMayorCast *cast, unsigned age,
                                 unsigned release_age, int active, int returning)
{
    if (!cast) return;
    cast->visible = active && age >= cast->emerge;
    if (!cast->visible) return;
    unsigned elapsed = age - cast->emerge;
    const Clip *clip = &cast->walking;
    cast->yaw = cast->facing;
    if (returning && release_age >= cast->depart) {
        elapsed = release_age - cast->depart;
        cast->yaw = heading_at(cast, elapsed);
        unsigned ticks = elapsed > cast->turn_ticks ? elapsed - cast->turn_ticks : 0;
        if (ticks >= cast->inward.ticks) {
            cast->visible = 0;
            return;
        }
        walk_to(cast->position, cast->porch, &cast->inward, ticks);
    } else if (elapsed < cast->outward.ticks) {
        walk_to(cast->position, cast->home, &cast->outward, elapsed);
    } else {
        elapsed -= cast->outward.ticks;
        clip = &cast->speaking;
        memcpy(cast->position, cast->porch, sizeof(cast->porch));
    }
    cast->pose = clip->poses[elapsed % clip->count];
}

const int16_t *bfm_village_mayor_cast_position(const BfmVillageMayorCast *cast)
{
    return cast && cast->visible ? cast->position : NULL;
}

int bfm_village_mayor_cast_add(const BfmVillageMayorCast *cast, BfmBedroomFigure *figure,
                               const BfmBedroomView *view)
{
    if (!cast || !figure || !view) return 0;
    if (!cast->visible) return 1;
    const int xyz[3] = { cast->position[0], cast->position[1], cast->position[2] };
    return bfm_bedroom_visitor_add(cast->body, figure, view, 0, xyz, (int)cast->yaw, cast->pose);
}
