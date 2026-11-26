/* SPDX-License-Identifier: MIT */
#include "forest_entry.h"

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}

static uint32_t read32(const uint8_t *p)
{
    return read16(p) | (uint32_t)read16(p + 2) << 16;
}

static const uint8_t *record(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base || (address - base) % 4 || count > bytes || address - base > bytes - count)
        return NULL;
    return scene + (address - base);
}

int bfm_forest_entry_from_scene(const uint8_t *scene, size_t bytes, unsigned member,
                                RoomTransferDescriptor *out)
{
    if (!scene || !out || member > 1)
        return 0;
    const uint8_t *door;
    if (member == 0) {
        /* The moon sequence queues its own explicit transfer on Cross. Its
         * regular door list contains palace destinations, not this transfer. */
        door = record(scene, bytes, UINT32_C(0x8018263c), 36);
    } else {
        /* The stairwell skip selects the second door in its active room. */
        const uint8_t *slot = record(scene, bytes, UINT32_C(0x8018380c), 4);
        const uint8_t *room = slot ? record(scene, bytes, read32(slot), 0xcc) : NULL;
        const uint8_t *list = room ? record(scene, bytes, read32(room + 0xc8), 8) : NULL;
        door = list ? record(scene, bytes, read32(list + 4), 36) : NULL;
    }
    if (!door)
        return 0;
    bfm_forest_decode_transfer(door, out);
    return 1;
}

void bfm_forest_decode_transfer(const uint8_t door[36], RoomTransferDescriptor *out)
{
    *out = (RoomTransferDescriptor){
        .room = read16(door), .parameter = door[2], .world = door[3],
        .resource = read32(door + 4), .room_value = read32(door + 8),
        .scene_data = read32(door + 12),
        .x = read16(door + 16), .y = read16(door + 18), .z = read16(door + 20),
        .facing = read16(door + 22), .activate = read32(door + 24),
        .reserved1c = read32(door + 28), .finish = read32(door + 32)
    };
}

BfmForestField *bfm_forest_enter(const MusashiDiscMedia *disc, const RoomTransferDescriptor *entry,
                                 ForestPlayerState *player, int16_t *model_yaw, int16_t cached_position[3])
{
    if (!entry || entry->room != 0x3008 || !player || !model_yaw || !cached_position || entry->world != 0)
        return NULL;
    BfmForestField *field = bfm_forest_field_open(disc, entry->room);
    if (!field || !bfm_forest_place_player(field, entry->parameter, player, model_yaw, cached_position)) {
        bfm_forest_field_free(field);
        return NULL;
    }
    return field;
}
