/* SPDX-License-Identifier: MIT */
#include "bakery_door.h"
#include "../pc_port/forest_player_bytes.h"

static const uint32_t scene_origin = UINT32_C(0x80128158);
static const uint32_t lower_room = UINT32_C(0x80192b18);

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (!scene || address < scene_origin || count > bytes) return NULL;
    size_t offset = address - scene_origin;
    return offset <= bytes - count ? scene + offset : NULL;
}

int bfm_bakery_door_is_open(const BfmSaveRecord *save)
{
    if (!save) return 0;
    unsigned hour = save->progress[0x39];
    return save->progress[0x42] != 3 && hour >= 7 && hour < 19;
}

const uint8_t *bfm_bakery_door_transfer(const uint8_t *scene, size_t bytes,
                                        uint16_t wall_contact, const BfmSaveRecord *save)
{
    if ((wall_contact & 31u) != 8 || !bfm_bakery_door_is_open(save)) return NULL;
    const uint8_t *room = mounted(scene, bytes, lower_room, 0xcc);
    if (!room) return NULL;
    /* Entry 4 also supplies the street station for the return from Jam. */
    const uint8_t *entries = mounted(scene, bytes, u32le(room + 0xc8), 5 * 4);
    const uint8_t *door = entries ? mounted(scene, bytes, u32le(entries + 4 * 4), 36) : NULL;
    if (!door || u16le(door) != 0x2015 || door[2] != 0) return NULL;
    return door;
}
