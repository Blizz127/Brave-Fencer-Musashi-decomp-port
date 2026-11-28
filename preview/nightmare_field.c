/* SPDX-License-Identifier: MIT */
#include "nightmare_field.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"

const BfmNightmareLayout *bfm_nightmare_layout(const BfmBedroomEntry *entry)
{
    static const BfmNightmareLayout early = {
        .number = 0x3012, .street_exit = 0x000a1010, .binding = 0x8017f278,
        .script = 0x80182f50, .camera = 0x8017ee04, .figure = 0x801810f0,
        .face = 0x80181e5c, .flash = 0x80182cb0, .landing_sounds = 0x80181dbc,
        .seated_body = 0x80182d58, .message = 0x801830b0, .window = 0x8017f994
    };
    static const BfmNightmareLayout illness = {
        .number = 0x3054, .street_exit = 0x000a1052, .binding = 0x8017f22c,
        .script = 0x80182f04, .camera = 0x8017edb8, .figure = 0x801810a4,
        .face = 0x80181e10, .flash = 0x80182c64, .landing_sounds = 0x80181d70,
        .seated_body = 0x80182d0c, .message = 0x80183058, .window = 0x8017f948
    };
    if (!entry) return NULL;
    uint32_t number = u32le(entry->state.destination);
    if (entry->scene_member == 3 && number == early.number) return &early;
    if (entry->scene_member == 126 && number == illness.number) return &illness;
    return NULL;
}

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || count > bytes || address - base > bytes - count) return NULL;
    return scene + (address - base);
}

int bfm_nightmare_entrance(const uint8_t *scene, size_t bytes, const BfmBedroomEntry *entry,
                           uint32_t *room_address, int16_t xyz[3], unsigned *yaw)
{
    const BfmNightmareLayout *layout = bfm_nightmare_layout(entry);
    if (!layout || !room_address || !xyz || !yaw) return 0;

    const uint8_t *slot = mounted(scene, bytes, layout->binding, 4);
    if (!slot) return 0;
    uint32_t address = u32le(slot);
    const uint8_t *room = mounted(scene, bytes, address, 0xcc);
    if (!room) return 0;
    const uint8_t *entrances = mounted(scene, bytes, u32le(room + 0xc8), 4);
    if (!entrances) return 0;
    const uint8_t *entrance = mounted(scene, bytes, u32le(entrances), 36);
    const uint8_t *script = mounted(scene, bytes, layout->script, 4);
    if (!entrance || u32le(entrance) != layout->number ||
        u32le(entrance + 8) != UINT32_C(0x8017e6c8) ||
        u32le(entrance + 24) != layout->camera ||
        !script || u32le(script) != UINT32_C(0x8017e77c)) return 0;

    *room_address = address;
    for (unsigned axis = 0; axis < 3; ++axis) xyz[axis] = s16le(entrance + 16 + 2 * axis);
    *yaw = u16le(entrance + 22) & 4095u;
    return 1;
}

BfmBedroomField *bfm_nightmare_field_open(const uint8_t *scene, size_t bytes,
                                          const BfmBedroomEntry *entry, uint32_t *room_address)
{
    uint32_t room;
    int16_t spawn[3];
    unsigned yaw;
    if (!room_address || !bfm_nightmare_entrance(scene, bytes, entry, &room, spawn, &yaw)) return NULL;

    BfmBedroomField *field = bfm_bedroom_field_load(scene, bytes, room, 500);
    if (!field) return NULL;
    /* The entrance camera starts at its requested pitch, azimuth and distance.
     * Its focus has zero local offset from Musashi. */
    BfmForestTransform orbit;
    bfm_forest_actor_pitch_yaw(568, 1536, spawn, &orbit);
    int eye[3], target[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        int product = 900 * orbit.rotation[axis][2];
        target[axis] = spawn[axis];
        eye[axis] = target[axis] + product / 4096 - (product % 4096 < 0);
    }
    bfm_bedroom_field_viewpoint(field, eye, target, 1, 1);
    *room_address = room;
    return field;
}
