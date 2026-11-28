/* SPDX-License-Identifier: MIT */
#include "village_mine_props.h"
#include "valley_hinge.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { SIGNS = 2, PLACEMENT_SIZE = 20 };
static const uint32_t base = UINT32_C(0x80128158);

typedef struct {
    int16_t position[3];
    BfmForestTransform root;
    const uint8_t *message;
    size_t length;
} MineSign;

typedef struct {
    uint32_t binding, room, kit, programs[SIGNS];
    unsigned sign_class;
} MineSignRoom;

struct BfmVillageMineProps {
    int yields_to_resident;
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose;
    BfmForestGuardBox box;
    MineSign signs[SIGNS];
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (!scene || address < base || length > bytes || address - base > bytes - length) return NULL;
    return scene + (address - base);
}

void bfm_village_mine_props_free(BfmVillageMineProps *props)
{
    if (props) {
        musashi_forest_player_mesh_free(props->mesh);
        free(props);
    }
}

static int load_sign(MineSign *sign, const uint8_t *row, const uint8_t *scene, size_t bytes,
                     uint32_t program)
{
    const uint8_t *script = mounted(scene, bytes, program, 24);
    if (!script || u32le(script) != 10 || u32le(script + 4) || u32le(script + 8) != 1 ||
        u32le(script + 16) != 2 || u32le(script + 20) ||
        s16le(row + 10) || s16le(row + 14)) return 0;
    uint32_t address = u32le(script + 12);
    const uint8_t *start = mounted(scene, bytes, address, 1);
    if (!start || *start != 5) return 0;
    const uint8_t *end = memchr(start, 0, bytes - (address - base));
    if (!end) return 0;
    sign->message = start;
    sign->length = (size_t)(end - start) + 1;
    for (unsigned i = 0; i < 3; ++i) sign->position[i] = s16le(row + 2*i);
    bfm_forest_actor_root(u16le(row + 12), sign->position, &sign->root);
    return 1;
}

static BfmVillageMineProps *open_props(const uint8_t *scene, size_t bytes, const MineSignRoom *layout)
{
    const uint8_t *binding = mounted(scene, bytes, layout->binding, 4);
    if (!binding || u32le(binding) != layout->room) return NULL;
    const uint8_t *room = mounted(scene, bytes, u32le(binding), 0xbc);
    const uint8_t *kit = mounted(scene, bytes, layout->kit, 12);
    if (!room || !kit || !(u16le(kit + 2) & 0x0800)) return NULL;
    const uint8_t *shape = mounted(scene, bytes, u32le(kit + 8), 16);
    if (!shape || u32le(shape)) return NULL;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end - first) % PLACEMENT_SIZE) return NULL;
    const uint8_t *rows = mounted(scene, bytes, first, end - first);
    if (!rows) return NULL;
    BfmVillageMineProps *props = calloc(1, sizeof *props);
    if (!props) return NULL;
    props->yields_to_resident = layout->sign_class == 834;
    for (unsigned i = 0; i < 3; ++i) {
        props->box.low[i] = s16le(shape + 4 + 4*i);
        props->box.high[i] = s16le(shape + 6 + 4*i);
        if (props->box.low[i] > props->box.high[i]) goto fail;
    }
    for (size_t off = 0; off < end - first; off += PLACEMENT_SIZE) {
        const uint8_t *row = rows + off;
        unsigned kind = u16le(row + 6);
        if (!kind) break;
        if (kind != layout->sign_class) continue;
        unsigned variant = u16le(row + 8);
        if (variant >= SIGNS || props->signs[variant].message ||
            !load_sign(&props->signs[variant], row, scene, bytes, layout->programs[variant])) goto fail;
    }
    for (unsigned i = 0; i < SIGNS; ++i)
        if (!props->signs[i].message) goto fail;
    unsigned parts = 0;
    props->mesh = musashi_forest_player_mesh_from_table(scene, bytes, base, u32le(kit + 4), &parts);
    if (!props->mesh || parts != 1) goto fail;
    const int16_t origin[3] = {0};
    bfm_forest_actor_root(0, origin, &props->pose.local[0]);
    return props;

fail:
    bfm_village_mine_props_free(props);
    return NULL;
}

BfmVillageMineProps *bfm_village_mine_props_open(const uint8_t *scene, size_t bytes)
{
    const MineSignRoom gallery = {
        .binding = 0x8018245cu, .room = 0x8018615cu, .kit = 0x801860e8u,
        .programs = {0x8018611cu, 0x80186134u}, .sign_class = 834
    };
    return open_props(scene, bytes, &gallery);
}

BfmVillageMineProps *bfm_village_mine_props_open_shaft(const uint8_t *scene, size_t bytes)
{
    const uint8_t *programs = mounted(scene, bytes, 0x80181f10u, SIGNS * 4);
    if (!programs) return NULL;
    const MineSignRoom shaft = {
        .binding = 0x8017e300u, .room = 0x80181f28u, .kit = 0x80181eacu,
        .programs = {u32le(programs), u32le(programs + 4)}, .sign_class = 835
    };
    return open_props(scene, bytes, &shaft);
}

int bfm_village_mine_props_draw(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    const BfmVillageMineProps *props = context;
    if (!props || !figure || !view) return 0;
    BfmForestActor actor = {.mesh = props->mesh, .pose = &props->pose, .visible_parts = 1};
    for (unsigned i = 0; i < SIGNS; ++i) {
        actor.root = &props->signs[i].root;
        if (!bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0)) return 0;
    }
    return 1;
}

int bfm_village_mine_props_body(const BfmVillageMineProps *props, const int16_t previous[3],
                                int16_t foot[3], unsigned tick, int airborne)
{
    int contact = 0;
    if (props) for (unsigned i = 0; i < SIGNS; ++i)
        contact |= bfm_valley_box_block_body(&props->box, &props->signs[i].root,
                                             previous, foot, tick, airborne);
    return contact;
}

static int64_t distance_squared(const int16_t a[3], const int16_t b[3])
{
    int64_t result = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t delta = (int)a[i] - b[i];
        result += delta * delta;
    }
    return result;
}

BfmForestMessage *bfm_village_mine_props_read(const BfmVillageMineProps *props,
    const int16_t foot[3], int square_pressed, const int16_t actor548[3], int16_t point[3])
{
    if (!props || !foot || !square_pressed) return NULL;
    for (unsigned i = 0; i < SIGNS; ++i) {
        const MineSign *sign = &props->signs[i];
        if (distance_squared(foot, sign->position) > 64 * 64) continue;
        if (props->yields_to_resident && actor548 &&
            distance_squared(actor548, sign->position) < 64 * 64) continue;
        if (point) memcpy(point, sign->position, sizeof sign->position);
        return bfm_field_message_borrow(sign->message, sign->length);
    }
    return NULL;
}
