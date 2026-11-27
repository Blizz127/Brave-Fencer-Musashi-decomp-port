/* SPDX-License-Identifier: MIT */
#include "village_jon_actor.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { JON_CLASS = 173, JON_PARTS = 5 };
static const uint32_t scene_origin = UINT32_C(0x80128158);
static const uint32_t lower_grillin = UINT32_C(0x80192b18);

struct BfmVillageJonActor {
    const uint8_t *scene, *keys;
    size_t bytes;
    MusashiForestPlayerMesh *mesh;
    int16_t station[3];
    unsigned yaw, frame, frames;
};

static const uint8_t *mounted(const BfmVillageJonActor *actor, uint32_t address, size_t size)
{
    if (address < scene_origin) return NULL;
    size_t offset = address - scene_origin;
    return offset <= actor->bytes && size <= actor->bytes - offset ? actor->scene + offset : NULL;
}

static int has_stocks_actor(const BfmVillageJonActor *actor)
{
    const uint8_t *room = mounted(actor, lower_grillin, 0xbc);
    if (!room) return 0;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end - first) % 20) return 0;
    const uint8_t *rows = mounted(actor, first, end - first);
    if (!rows) return 0;
    unsigned count = 0;
    for (uint32_t offset = 0; offset < end - first; offset += 20) {
        const uint8_t *row = rows + offset;
        uint16_t kind = u16le(row + 6);
        if (!kind) return count == 1;
        if (kind == JON_CLASS && !u16le(row + 8)) ++count;
    }
    return 0;
}

static int load_pose_keys(BfmVillageJonActor *actor)
{
    /* The stocks use a dedicated clip; the generic village walking profile
     * describes other actions for this character. Each key lasts one update. */
    uint32_t address = UINT32_C(0x801c3c90);
    actor->keys = mounted(actor, address, 8);
    for (unsigned i = 0; i < 128; ++i) {
        const uint8_t *row = mounted(actor, address + 8*i, 8);
        if (!row) return 0;
        uint32_t key = u32le(row), duration = u32le(row + 4);
        if (!key) {
            actor->frames = i;
            return i && duration == 0xffff;
        }
        if ((key >> 24) != 0x81 || (key & 3) || duration != 1 ||
            !mounted(actor, key & ~UINT32_C(0x01000000), 8*JON_PARTS)) return 0;
    }
    return 0;
}

BfmVillageJonActor *bfm_village_jon_actor_open(const uint8_t *scene, size_t bytes, uint32_t room)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY || room != lower_grillin) return NULL;
    BfmVillageJonActor *actor = calloc(1, sizeof *actor);
    if (!actor) return NULL;
    actor->scene = scene;
    actor->bytes = bytes;
    const uint8_t *station = mounted(actor, UINT32_C(0x80192a64), 8);
    const uint8_t *profile = mounted(actor, UINT32_C(0x80192a6c), 4);
    if (!station || !profile || !has_stocks_actor(actor) || !load_pose_keys(actor)) goto fail;
    for (unsigned axis = 0; axis < 3; ++axis) actor->station[axis] = s16le(station + 2*axis);
    actor->yaw = u16le(station + 6) & 4095;
    unsigned parts;
    actor->mesh = musashi_forest_player_mesh_from_table(scene, bytes, scene_origin,
                                                        u32le(profile), &parts);
    if (!actor->mesh || parts != JON_PARTS) goto fail;
    return actor;
fail:
    bfm_village_jon_actor_free(actor);
    return NULL;
}

void bfm_village_jon_actor_free(BfmVillageJonActor *actor)
{
    if (!actor) return;
    musashi_forest_player_mesh_free(actor->mesh);
    free(actor);
}

const int16_t *bfm_village_jon_actor_position(const BfmVillageJonActor *actor)
{
    return actor ? actor->station : NULL;
}

int bfm_village_jon_can_talk(const BfmVillageJonActor *actor, unsigned story,
                             const int16_t player[3])
{
    if (!actor || !player || story < 30 || story >= 70) return 0;
    int64_t distance_squared = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t delta = (int32_t)player[axis] - actor->station[axis];
        distance_squared += delta * delta;
    }
    return distance_squared <= 128*128;
}

void bfm_village_jon_actor_reset(BfmVillageJonActor *actor)
{
    if (actor) actor->frame = 0;
}

void bfm_village_jon_actor_step(BfmVillageJonActor *actor)
{
    if (actor && ++actor->frame == actor->frames) actor->frame = 0;
}

int bfm_village_jon_actor_add(const BfmVillageJonActor *actor, unsigned story,
                              BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!actor || !figure || !view) return 0;
    if (story >= 70) return 1;
    uint32_t key = u32le(actor->keys + 8*actor->frame) & ~UINT32_C(0x01000000);
    BfmForestPose pose;
    if (!bfm_forest_actor_pose_decode(mounted(actor, key, 8*JON_PARTS), 8*JON_PARTS,
                                      JON_PARTS, &pose)) return 0;
    BfmForestTransform root;
    bfm_forest_actor_root(actor->yaw, actor->station, &root);
    BfmForestActor body = {.mesh=actor->mesh, .pose=&pose, .root=&root,
                           .visible_parts=(1u << JON_PARTS)-1u};
    return bfm_bedroom_figure_add_actor(figure, view, &body, NULL, NULL, 0);
}
