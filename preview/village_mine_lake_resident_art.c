/* SPDX-License-Identifier: MIT */
#include "village_mine_lake_resident_art.h"
#include "tower_summit_departure.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum { DICEY = 27, BRISKET = 21, DEPARTURE_KEYS = 16 };
static const uint32_t scene_origin = UINT32_C(0x80128158);

typedef struct {
    const uint8_t *keys[DEPARTURE_KEYS];
    unsigned count;
} ResidentClip;

struct BfmMineLakeResidentArt {
    const MusashiForestPlayerMesh *model;
    unsigned parts;
    ResidentClip standing, departure;
};

static const uint8_t *at(const uint8_t *data, size_t bytes, uint32_t origin,
                         uint32_t address, size_t length)
{
    if (address < origin) return NULL;
    size_t offset = address - origin;
    if (offset > bytes || length > bytes - offset) return NULL;
    return data + offset;
}

/* Timed keys and the loop marker are in the room. The packed joint frames
 * they reference are in the separately loaded character resource. */
static int load_clip(ResidentClip *clip, const uint8_t *scene, size_t bytes, uint32_t address,
                     unsigned keys, unsigned parts, const uint8_t *resource,
                     size_t resource_bytes, uint32_t resource_origin)
{
    const uint8_t *tape = at(scene, bytes, scene_origin, address, 8 * (keys + 1));
    if (!tape || (address & 3u) || keys > DEPARTURE_KEYS ||
        u32le(tape + keys * 8) != 0 || u32le(tape + keys * 8 + 4) != UINT16_MAX) return 0;

    for (unsigned i = 0; i < keys; ++i) {
        uint32_t frame = u32le(tape + i * 8);
        if ((frame >> 24) != 0x81u || (frame & 3u) || u32le(tape + i * 8 + 4) != 1) return 0;
        clip->keys[i] = at(resource, resource_bytes, resource_origin,
                           frame & ~UINT32_C(0x01000000), 8 * parts);
        if (!clip->keys[i]) return 0;
    }
    clip->count = keys;
    return 1;
}

BfmMineLakeResidentArt *bfm_mine_lake_resident_art_load(const uint8_t *scene, size_t bytes,
    unsigned captive_id, const MusashiForestPlayerMesh *model,
    const uint8_t *resource, size_t resource_bytes, uint32_t model_va,
    uint32_t idle_clip_va, uint32_t departure_clip_va)
{
    if (!scene || !model || !resource || bytes > BFM_FIELD_SCENE_CAPACITY ||
        (captive_id != DICEY && captive_id != BRISKET)) return NULL;

    const uint8_t *binding = at(scene, bytes, scene_origin, UINT32_C(0x8019fb3c), 4);
    if (!binding || (model_va & 3u)) return NULL;
    unsigned parts = captive_id == DICEY ? 11 : 12;
    uint32_t origin = u32le(binding);
    const uint8_t *table = at(resource, resource_bytes, origin, model_va, 4 * (parts + 1));
    if (!table || u32le(table + 4 * parts) != 0 ||
        !musashi_forest_player_mesh_part(model, parts - 1) ||
        musashi_forest_player_mesh_part(model, parts)) return NULL;
    uint32_t previous = origin;
    for (unsigned i = 0; i < parts; ++i) {
        uint32_t part = u32le(table + 4 * i);
        if ((part & 3u) || part <= previous || part >= model_va) return NULL;
        previous = part;
    }

    BfmMineLakeResidentArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    art->model = model;
    art->parts = parts;
    unsigned standing_keys = captive_id == DICEY ? 10 : 12;
    if (!load_clip(&art->standing, scene, bytes, idle_clip_va, standing_keys,
                   parts, resource, resource_bytes, origin) ||
        !load_clip(&art->departure, scene, bytes, departure_clip_va, DEPARTURE_KEYS,
                   parts, resource, resource_bytes, origin)) {
        free(art);
        return NULL;
    }
    return art;
}

void bfm_mine_lake_resident_art_free(BfmMineLakeResidentArt *art)
{
    free(art);
}

int bfm_mine_lake_resident_art_draw(const BfmMineLakeResidentArt *art,
    const BfmBedroomRoot *root, int departure_clip, unsigned pose_tick,
    int effect_active, unsigned effect_tick,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!art || !root || !figure || !view) return 0;
    const ResidentClip *clip = departure_clip ? &art->departure : &art->standing;
    BfmForestPose pose;
    if (!bfm_forest_actor_pose_decode(clip->keys[pose_tick % clip->count], 8 * art->parts,
                                      art->parts, &pose)) return 0;
    BfmForestTransform transform;
    bfm_forest_actor_root(root->yaw, root->xyz, &transform);
    if (effect_active) bfm_tower_summit_departure_hero(effect_tick, &transform);
    const BfmForestActor actor = {.mesh=art->model, .pose=&pose, .root=&transform,
                                  .visible_parts=(UINT32_C(1) << art->parts) - 1};
    return bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0);
}
