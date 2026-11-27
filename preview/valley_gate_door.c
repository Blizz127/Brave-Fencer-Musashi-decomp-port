/* SPDX-License-Identifier: MIT */
#include "valley_gate_door.h"
#include "valley_debris_art.h"
#include "woods_fusion_art.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum { LEAVES = 2, SLIDE_FIELDS = 64, PUFFS_PER_LEAF = 11, PUFF_CAPACITY = 128 };
static const uint32_t scene_base = UINT32_C(0x80128158);

typedef enum { WAITING, BEGIN_SLIDE, SLIDING, OPENED, RESTORED } GatePhase;

typedef struct {
    MusashiForestPlayerMesh *mesh;
    BfmForestTransform root;
    BfmForestGuardBox box;
    unsigned flags;
    int16_t step_x;
} GateLeaf;

typedef struct {
    int16_t xyz[3];
    uint8_t scale, cel, remaining;
} GatePuff;

struct BfmValleyGateDoor {
    GateLeaf leaf[LEAVES];
    BfmForestGuardBox restored_box;
    BfmForestPose pose;
    BfmValleyDebrisArt art;
    GatePuff puff[PUFF_CAPACITY];
    GatePhase phase;
    unsigned elapsed, active_puffs;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (address < scene_base || count > bytes || address - scene_base > bytes - count) return NULL;
    return scene + (address - scene_base);
}

static int read_box(BfmForestGuardBox *box, const uint8_t *scene, size_t bytes, uint32_t address)
{
    const uint8_t *row = mounted(scene, bytes, address, 16);
    if (!row || u32le(row)) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        box->low[i] = s16le(row + 4 + i * 4);
        box->high[i] = s16le(row + 6 + i * 4);
        if (box->low[i] >= box->high[i]) return 0;
    }
    return 1;
}

void bfm_valley_gate_door_free(BfmValleyGateDoor *door)
{
    if (!door) return;
    for (unsigned i = 0; i < LEAVES; ++i) musashi_forest_player_mesh_free(door->leaf[i].mesh);
    free(door);
}

BfmValleyGateDoor *bfm_valley_gate_door_open(const uint8_t *scene, size_t bytes, unsigned stage)
{
    if (!scene) return NULL;
    const uint8_t *binding = mounted(scene, bytes, UINT32_C(0x8018231c), 4);
    if (!binding || u32le(binding) != UINT32_C(0x80186620)) return NULL;
    const uint8_t *placements = mounted(scene, bytes, UINT32_C(0x80186af8), 20 * LEAVES);
    const uint8_t *profiles = mounted(scene, bytes, UINT32_C(0x80186218), 52 * LEAVES);
    const uint8_t *velocities = mounted(scene, bytes, UINT32_C(0x80186280), 12 * LEAVES);
    if (!placements || !profiles || !velocities) return NULL;

    BfmValleyGateDoor *door = calloc(1, sizeof *door);
    if (!door) return NULL;
    if (!read_box(&door->restored_box, scene, bytes, UINT32_C(0x80186208)) ||
        !bfm_valley_debris_art_load(&door->art, scene, bytes)) goto fail;
    if (stage >= 160) door->phase = RESTORED;
    const int16_t origin[3] = {0};
    bfm_forest_actor_root(0, origin, &door->pose.local[0]);
    for (unsigned i = 0; i < LEAVES; ++i) {
        GateLeaf *leaf = &door->leaf[i];
        const uint8_t *row = placements + 20 * i, *profile = profiles + 52 * i;
        const uint8_t *velocity = velocities + 12 * i;
        if (u16le(row + 6) != 229 || u16le(row + 8) != i ||
            u16le(row + 10) || u16le(row + 12) || u16le(row + 14) ||
            u32le(velocity) != (i ? UINT32_C(0xffff0000) : UINT32_C(0x10000)) ||
            u32le(velocity + 4) || u32le(velocity + 8)) goto fail;
        leaf->flags = u16le(profile + 2);
        leaf->step_x = s16le(velocity + 2);
        if (!read_box(&leaf->box, scene, bytes, u32le(profile + 8))) goto fail;
        unsigned parts;
        leaf->mesh = musashi_forest_player_mesh_from_table(scene, bytes, scene_base,
                                                           u32le(profile + 4), &parts);
        if (!leaf->mesh || parts != 1) goto fail;
        int16_t xyz[3] = {s16le(row), s16le(row + 2), s16le(row + 4)};
        bfm_forest_actor_root(0, xyz, &leaf->root);
        if (door->phase == RESTORED) leaf->root.translation[0] += SLIDE_FIELDS * leaf->step_x;
    }
    return door;
fail:
    bfm_valley_gate_door_free(door);
    return NULL;
}

unsigned bfm_valley_gate_door_shape(const BfmValleyGateDoor *door, unsigned leaf,
    const BfmForestGuardBox **box, const BfmForestTransform **root)
{
    if (!door || leaf >= LEAVES || !box || !root) return 0;
    if (door->phase == RESTORED && leaf == 0) {
        *box = &door->restored_box;
        *root = &door->leaf[leaf].root;
        return 0x0800;
    }
    if (door->phase != WAITING) return 0;
    *box = &door->leaf[leaf].box;
    *root = &door->leaf[leaf].root;
    return door->leaf[leaf].flags;
}

static void advance_puffs(BfmValleyGateDoor *door)
{
    if (!door->active_puffs) return;
    for (unsigned i = 0; i < PUFF_CAPACITY; ++i) {
        GatePuff *puff = &door->puff[i];
        if (!puff->remaining || --puff->remaining) continue;
        if (++puff->cel < BFM_VALLEY_DEBRIS_CELS)
            puff->remaining = door->art.duration[puff->cel];
        else
            --door->active_puffs;
    }
}

static void emit_puffs(BfmValleyGateDoor *door, BfmGameplayRun *run)
{
    unsigned slot = 0;
    for (unsigned i = 0; i < LEAVES; ++i) {
        const BfmForestTransform *root = &door->leaf[i].root;
        for (unsigned n = 0; n < PUFFS_PER_LEAF; ++n) {
            while (slot < PUFF_CAPACITY && door->puff[slot].remaining) ++slot;
            if (slot == PUFF_CAPACITY) return;
            door->puff[slot++] = (GatePuff){
                .xyz = {(int16_t)(root->translation[0] + (i ? 128 : -128)), -514,
                        (int16_t)(root->translation[2] - 640 + (int)n * 128)},
                .scale = (uint8_t)(2 + bfm_gameplay_random(run) % 4),
                .remaining = door->art.duration[0]
            };
            ++door->active_puffs;
        }
    }
}

BfmValleyGateDoorEvents bfm_valley_gate_door_step(BfmValleyGateDoor *door,
    BfmGameplayRun *run, int soldier_landed)
{
    BfmValleyGateDoorEvents event = {0};
    if (!door || !run) return event;
    advance_puffs(door);
    switch (door->phase) {
    case WAITING:
        if (soldier_landed) {
            door->phase = BEGIN_SLIDE;
            event.shake = 1;
        }
        break;
    case BEGIN_SLIDE:
        door->phase = SLIDING;
        event.slide_sounds = LEAVES;
        break;
    case SLIDING:
        for (unsigned i = 0; i < LEAVES; ++i)
            door->leaf[i].root.translation[0] += door->leaf[i].step_x;
        ++door->elapsed;
        if (door->elapsed % 8 == 0) emit_puffs(door, run);
        if (door->elapsed == SLIDE_FIELDS) door->phase = OPENED;
        break;
    case OPENED:
    case RESTORED:
        break;
    }
    return event;
}

int bfm_valley_gate_door_draw(void *ctx, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    const BfmValleyGateDoor *door = ctx;
    if (!door || !figure || !view) return 0;
    for (unsigned i = 0; i < LEAVES; ++i) {
        BfmForestActor actor = {.mesh = door->leaf[i].mesh, .pose = &door->pose,
                                .root = &door->leaf[i].root, .visible_parts = 1};
        if (!bfm_bedroom_figure_add_far_unlit_actor(figure, view, &actor)) return 0;
    }
    if (!door->active_puffs) return 1;
    BfmForestCamera camera;
    if (!bfm_woods_effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[PUFF_CAPACITY];
    unsigned count = 0;
    for (unsigned i = 0; i < PUFF_CAPACITY; ++i) {
        const GatePuff *puff = &door->puff[i];
        if (puff->remaining)
            count += bfm_valley_debris_art_project_scaled(&door->art, puff->cel, puff->scale,
                                                         puff->xyz, &camera, i, &cards[count]);
    }
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}
