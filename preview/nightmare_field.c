/* SPDX-License-Identifier: MIT */
#include "nightmare_field.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || count > bytes || address - base > bytes - count) return NULL;
    return scene + (address - base);
}

int bfm_nightmare_entrance(const uint8_t *scene, size_t bytes, const BfmBedroomEntry *entry,
                           uint32_t *room_address, int16_t xyz[3], unsigned *yaw)
{
    if (!entry || !room_address || !xyz || !yaw || entry->scene_member != 3 ||
        u32le(entry->state.destination) != 0x3012) return 0;

    const uint8_t *slot = mounted(scene, bytes, UINT32_C(0x8017f278), 4);
    if (!slot) return 0;
    uint32_t address = u32le(slot);
    const uint8_t *room = mounted(scene, bytes, address, 0xcc);
    if (!room) return 0;
    const uint8_t *entrances = mounted(scene, bytes, u32le(room + 0xc8), 4);
    if (!entrances) return 0;
    const uint8_t *entrance = mounted(scene, bytes, u32le(entrances), 36);
    const uint8_t *script = mounted(scene, bytes, UINT32_C(0x80182f50), 4);
    if (!entrance || u32le(entrance) != 0x3012 ||
        u32le(entrance + 8) != UINT32_C(0x8017e6c8) ||
        u32le(entrance + 24) != UINT32_C(0x8017ee04) ||
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
