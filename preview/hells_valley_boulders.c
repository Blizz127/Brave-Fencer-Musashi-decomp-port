/* SPDX-License-Identifier: MIT */
#include "hells_valley_boulders.h"
#include "valley_rock_fall.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { ROLLING_ROCKS = 5, HINGE_RIDER = ROLLING_ROCKS + 1, MODEL_COUNT = 3 };
static const uint32_t scene_base = UINT32_C(0x80128158);

typedef enum {
    ROCK_WAITING, ROCK_START_ROLL, ROCK_ROLLING, ROCK_START_FALL, ROCK_FALLING, ROCK_SETTLED
} RockPhase;

typedef struct {
    unsigned model, flag, pitch, yaw, ticks;
    RockPhase phase;
    int16_t xyz[3], landing[3], probe[3];
    BfmValleyRockFall fall;
    int32_t position[3], velocity[3];
    BfmForestTransform root;
} ValleyRock;

struct BfmHellsValleyBoulders {
    MusashiForestPlayerMesh *mesh[MODEL_COUNT];
    ValleyRock rock[ROLLING_ROCKS];
    BfmValleyHinge hinge;
    BfmValleyRockSupport support;
    BfmForestPose pose;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (address < scene_base || address - scene_base > bytes ||
        count > bytes - (address - scene_base)) return NULL;
    return scene + (address - scene_base);
}

void bfm_hells_valley_boulders_free(BfmHellsValleyBoulders *rocks)
{
    if (!rocks) return;
    for (unsigned i = 0; i < MODEL_COUNT; ++i)
        musashi_forest_player_mesh_free(rocks->mesh[i]);
    free(rocks);
}

BfmHellsValleyBoulders *bfm_hells_valley_boulders_open(const uint8_t *scene,
    size_t bytes, const uint8_t flags[64])
{
    if (!scene || !flags) return NULL;
    const uint8_t *binding = mounted(scene, bytes, UINT32_C(0x8018231c), 4);
    if (!binding || u32le(binding) != UINT32_C(0x80186620)) return NULL;
    const uint8_t *room = mounted(scene, bytes, u32le(binding), 0xb8);
    const uint8_t *profiles = mounted(scene, bytes, UINT32_C(0x80186000), 52 * MODEL_COUNT);
    const uint8_t *settled_flags = mounted(scene, bytes, UINT32_C(0x801860ac), 4 * ROLLING_ROCKS);
    const uint8_t *landings = mounted(scene, bytes, UINT32_C(0x801860c4), 8 * ROLLING_ROCKS);
    const uint8_t *probes = mounted(scene, bytes, UINT32_C(0x8018609c), 8 * 2);
    if (!room || !profiles || !settled_flags || !landings || !probes) return NULL;

    BfmHellsValleyBoulders *rocks = calloc(1, sizeof *rocks);
    if (!rocks) return NULL;
    if (!bfm_valley_rock_support_load(&rocks->support, scene, bytes) ||
        !bfm_valley_hinge_load(&rocks->hinge, scene, bytes, flags)) goto fail;
    for (unsigned i = 0; i < MODEL_COUNT; ++i) {
        unsigned parts;
        rocks->mesh[i] = musashi_forest_player_mesh_from_table(
            scene, bytes, scene_base, u32le(profiles + 52 * i + 4), &parts);
        if (!rocks->mesh[i] || parts != 1) goto fail;
    }
    const int16_t origin[3] = {0};
    bfm_forest_actor_root(0, origin, &rocks->pose.local[0]);
    unsigned seen = 0;
    uint32_t address = u32le(room + 0xb4);
    for (;;) {
        const uint8_t *row = mounted(scene, bytes, address, 20);
        if (!row) goto fail;
        unsigned type = u16le(row + 6);
        if (!type) break;
        address += 20;
        if (type != 228) continue;

        /* The low nibble selects the model profile. The high nibble selects
         * the flag and landing for this particular rock. */
        unsigned variant = u16le(row + 8), id = variant >> 12, model = variant & 15;
        if (id >= HINGE_RIDER || (seen & (1u << id))) goto fail;
        seen |= 1u << id;
        if (id == ROLLING_ROCKS) {
            if (model != 2) goto fail;
            continue;
        }
        if (model >= 2) goto fail;
        unsigned flag = u32le(settled_flags + 4 * id);
        if (flag >= 512) goto fail;
        int landed = (flags[flag / 8] & (1u << (flag % 8))) != 0;
        const uint8_t *position = landed ? landings + 8 * id : row;
        unsigned pitch = u16le(row + 10);
        ValleyRock *rock = &rocks->rock[id];
        rock->model = model;
        rock->flag = flag;
        rock->pitch = pitch;
        rock->yaw = u16le(row + 12);
        rock->phase = landed ? ROCK_SETTLED : ROCK_WAITING;
        for (unsigned axis = 0; axis < 3; ++axis) {
            rock->probe[axis] = s16le(probes + 8 * model + 2 * axis);
            rock->landing[axis] = s16le(landings + 8 * id + 2 * axis);
            rock->xyz[axis] = s16le(position + 2 * axis);
            rock->position[axis] = (int32_t)rock->xyz[axis] * 65536;
        }
        bfm_forest_actor_pitch_yaw(rock->pitch, rock->yaw, rock->xyz, &rock->root);
    }
    if (seen == (1u << HINGE_RIDER) - 1u) return rocks;
fail:
    bfm_hells_valley_boulders_free(rocks);
    return NULL;
}

static void update_transform(ValleyRock *rock)
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        int32_t pos = rock->position[axis];
        rock->xyz[axis] = (int16_t)(pos / 65536 - (pos % 65536 < 0));
    }
    bfm_forest_actor_pitch_yaw(rock->pitch, rock->yaw, rock->xyz, &rock->root);
}

/* The quake reaches every rock whose origin projects into the expanded
 * viewport. It can move both sides of a pit on the same field. */
static int in_quake_view(const BfmBedroomView *view, const int16_t xyz[3])
{
    int screen[3];
    bfm_bedroom_project(view, xyz, screen);
    return abs(screen[0] - 160) <= 170 && abs(screen[1] - 120) <= 130;
}

BfmValleyBoulderEvents bfm_hells_valley_boulders_step(BfmHellsValleyBoulders *rocks,
    const BfmBedroomView *view, const BfmVillageGround *ground, int earth_quake, const int16_t foot[3],
    uint8_t saved_flags[64], uint8_t live_flags[64])
{
    BfmValleyBoulderEvents events = {0};
    if (!rocks || !view || !ground || !saved_flags) return events;
    for (unsigned i = 0; i < ROLLING_ROCKS; ++i) {
        ValleyRock *rock = &rocks->rock[i];
        switch (rock->phase) {
        case ROCK_WAITING:
            if (earth_quake && in_quake_view(view, rock->xyz))
                rock->phase = ROCK_START_ROLL;
            break;
        case ROCK_START_ROLL:
            for (unsigned axis = 0; axis < 3; ++axis)
                rock->velocity[axis] = -32 * rock->root.rotation[axis][2];
            rock->ticks = 65;
            rock->phase = ROCK_ROLLING;
            events.started |= 1u << i;
            break;
        case ROCK_ROLLING:
            for (unsigned axis = 0; axis < 3; ++axis)
                rock->position[axis] += rock->velocity[axis];
            rock->pitch += 16;
            --rock->ticks;
            if (rock->ticks == 56) events.shake |= 1u << i;
            if (!rock->ticks) {
                rock->pitch = 1024;
                rock->phase = ROCK_START_FALL;
                events.left_cliff |= 1u << i;
            }
            update_transform(rock);
            break;
        case ROCK_START_FALL:
            bfm_valley_rock_fall_begin(&rock->fall, rock->position, rock->landing);
            rock->phase = ROCK_FALLING;
            break;
        case ROCK_FALLING:
            if (bfm_valley_rock_fall_step(&rock->fall, ground, rock->probe, rock->position)) {
                unsigned bit = 1u << (rock->flag % 8);
                saved_flags[rock->flag / 8] |= bit;
                if (live_flags) live_flags[rock->flag / 8] |= bit;
                events.landed |= 1u << i;
                rock->phase = ROCK_SETTLED;
            }
            update_transform(rock);
            break;
        case ROCK_SETTLED:
            break;
        }
    }
    BfmValleyHinge *hinge = &rocks->hinge;
    events.hinge = bfm_valley_hinge_step(hinge, earth_quake && in_quake_view(view, hinge->pivot), foot);
    if (events.hinge.settled) {
        unsigned bit = 1u << (hinge->flag % 8);
        saved_flags[hinge->flag / 8] |= bit;
        if (live_flags) live_flags[hinge->flag / 8] |= bit;
    }
    return events;
}

int bfm_hells_valley_hinge_foot(const BfmHellsValleyBoulders *rocks, unsigned rider,
    int short_probe, const int16_t from[3], int16_t to[3], unsigned *attached, BfmForestContact *surface)
{
    const BfmValleyHinge *hinge = &rocks->hinge;
    int16_t start[3], end[3], hit[3], normal[3];
    if ((rider && rider != HINGE_RIDER) || hinge->body.translation[1] < to[1]) return 0;
    int stationary = !memcmp(from, to, sizeof start);
    short_probe |= stationary;
    memcpy(start, short_probe && !rider ? to : from, sizeof start);
    memcpy(end, to, sizeof end);
    if (short_probe || rider) {
        start[1] -= 3;
        end[1] += short_probe ? 5 : 8;
    }
    if (!bfm_valley_hinge_contact(hinge, start, end, hit, normal)) return 0;
    if (!(rider && short_probe) && !bfm_valley_hinge_slide(hinge, start, end, to)) return 0;
    /* A horizontal sweep with no prior rider corrects XYZ, without acquiring
     * the actor. A short query on an existing rider just confirms the foot. */
    *attached = rider || short_probe || from[1] != end[1] ? HINGE_RIDER : 0;
    *surface = (BfmForestContact){.height = to[1],
        .normal = {normal[0], normal[1], normal[2]}, .slope = 0x2000};
    return 1;
}

int bfm_hells_valley_hinge_shadow(const BfmHellsValleyBoulders *rocks, const int16_t foot[3],
    int16_t position[3], int16_t normal[3])
{
    int16_t start[3] = {foot[0], (int16_t)(foot[1] - 16), foot[2]};
    int16_t end[3] = {start[0], (int16_t)(start[1] + 768), start[2]};
    return bfm_valley_hinge_contact(&rocks->hinge, start, end, position, normal);
}

unsigned bfm_hells_valley_boulders_support(const BfmHellsValleyBoulders *rocks,
    BfmValleyRockProbe kind, const int16_t from[3], const int16_t to[3], BfmForestContact *surface)
{
    unsigned nearest = 0;
    for (unsigned i = 0; i < ROLLING_ROCKS; ++i) {
        const ValleyRock *rock = &rocks->rock[i];
        BfmForestContact hit;
        if (rock->phase == ROCK_SETTLED && bfm_valley_rock_support_probe(&rocks->support,
                rock->model, &rock->root, kind, from, to, &hit) &&
                (!nearest || hit.height < surface->height)) {
            *surface = hit;
            nearest = i + 1;
        }
    }
    return nearest;
}

int bfm_hells_valley_hinge_body(const BfmHellsValleyBoulders *rocks, unsigned rider,
    const int16_t previous[3], int16_t foot[3], unsigned field_tick, int airborne)
{
    return rocks && rider != HINGE_RIDER &&
        bfm_valley_hinge_block_body(&rocks->hinge, previous, foot, field_tick, airborne);
}

int bfm_hells_valley_boulders_draw(void *context, BfmBedroomFigure *figure,
                                   const BfmBedroomView *view)
{
    const BfmHellsValleyBoulders *rocks = context;
    if (!rocks || !figure || !view) return 0;
    for (unsigned i = 0; i < HINGE_RIDER; ++i) {
        int hinge = i == ROLLING_ROCKS;
        BfmForestActor actor = {
            .mesh = rocks->mesh[hinge ? 2 : rocks->rock[i].model], .pose = &rocks->pose,
            .root = hinge ? &rocks->hinge.model : &rocks->rock[i].root, .visible_parts = 1
        };
        if (!bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0)) return 0;
    }
    return 1;
}
