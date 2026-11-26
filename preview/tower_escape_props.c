/* SPDX-License-Identifier: MIT */
#include "tower_escape_props.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum { ROCK, SHORT_TRUNK, LONG_TRUNK, SHAPES };
typedef enum { WAITING, ON_ROAD, PASSED } RoadState;

typedef struct {
    unsigned shape;
    RoadState state;
    BfmForestTransform root;
} RoadProp;

struct BfmTowerEscapeProps {
    MusashiForestPlayerMesh *mesh[SHAPES];
    BfmForestPose rest;
    RoadProp items[BFM_TOWER_ESCAPE_PROPS];
    size_t count;
    int previous_head_z;
};

BfmTowerEscapeProps *bfm_tower_escape_props_open(const BfmForestField *field)
{
    static const uint32_t origin = UINT32_C(0x80128158);
    static const uint32_t tables[SHAPES] = {
        UINT32_C(0x801e0294), UINT32_C(0x801e071c), UINT32_C(0x801e2164)
    };
    enum { ROW_BYTES = 12, ROW_COUNT = 26 };
    const size_t placements = UINT32_C(0x8018f07c) - origin;
    size_t size = 0;
    const uint8_t *scene = bfm_forest_field_scene(field, &size);
    if (!scene || placements > size || size - placements < ROW_COUNT * ROW_BYTES) return NULL;
    BfmTowerEscapeProps *props = calloc(1, sizeof *props);
    if (!props) return NULL;

    /* The shared single-frame rest pose has no local translation or rotation. */
    const int16_t zero[3] = {0};
    bfm_forest_actor_root(0, zero, &props->rest.local[0]);
    props->previous_head_z = INT16_MAX;
    for (size_t i = 0; i < ROW_COUNT; ++i) {
        const uint8_t *row = scene + placements + i * ROW_BYTES;
        unsigned kind = u16le(row + 6);
        if (kind != 69 && kind != 70) continue;
        if (props->count == BFM_TOWER_ESCAPE_PROPS) goto failed;
        unsigned shape = ROCK;
        if (kind == 70) shape = u16le(row + 8) ? LONG_TRUNK : SHORT_TRUNK;
        if (!props->mesh[shape]) {
            unsigned parts = 0;
            props->mesh[shape] = musashi_forest_player_mesh_from_table(scene, size, origin,
                                                                       tables[shape], &parts);
            if (!props->mesh[shape] || parts != 1) goto failed;
        }
        RoadProp *prop = &props->items[props->count++];
        prop->shape = shape;
        int16_t position[3] = {s16le(row), s16le(row + 2), s16le(row + 4)};
        bfm_forest_actor_root(0, position, &prop->root);
    }
    if (props->count == BFM_TOWER_ESCAPE_PROPS) return props;
failed:
    bfm_tower_escape_props_free(props);
    return NULL;
}

void bfm_tower_escape_props_free(BfmTowerEscapeProps *props)
{
    if (!props) return;
    for (unsigned shape = 0; shape < SHAPES; ++shape)
        musashi_forest_player_mesh_free(props->mesh[shape]);
    free(props);
}

void bfm_tower_escape_props_tick(BfmTowerEscapeProps *props, int16_t head_z)
{
    if (!props) return;
    for (size_t i = 0; i < props->count; ++i) {
        RoadProp *prop = &props->items[i];
        int z = prop->root.translation[2];
        /* Sweep the 16-unit spawn interval during the head's descent. This
         * also reconstructs the current visible set on the first update. */
        if (prop->state == WAITING && props->previous_head_z > z + 1024 && head_z <= z + 1040)
            prop->state = ON_ROAD;
        /* The standing trunk ends here. Its subsequent airborne tumble is
         * a separate moving actor state. */
        int reach = prop->shape == ROCK ? 128 : 16;
        if (prop->state == ON_ROAD && head_z < z + reach) prop->state = PASSED;
    }
    props->previous_head_z = head_z;
}

size_t bfm_tower_escape_props_actors(const BfmTowerEscapeProps *props,
                                     BfmForestActor *out, size_t capacity)
{
    if (!props || !out) return 0;
    size_t count = 0;
    for (size_t i = 0; i < props->count && count < capacity; ++i) {
        const RoadProp *prop = &props->items[i];
        if (prop->state != ON_ROAD) continue;
        out[count++] = (BfmForestActor){
            .mesh = props->mesh[prop->shape], .pose = &props->rest,
            .root = &prop->root, .visible_parts = 1
        };
    }
    return count;
}
