/* SPDX-License-Identifier: MIT */
#include "tower_ascent_door.h"
#include "forest_entry.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_tower_ascent_door_load(BfmTowerAscentDoor *door, const BfmForestField *room)
{
    if (!door || !room) return 0;
    const uint8_t *slot = bfm_forest_room_bytes(room, UINT32_C(0x8018350c), 4);
    const uint8_t *descriptor = slot ? bfm_forest_room_bytes(room, u32le(slot), 0xcc) : NULL;
    const uint8_t *destination = bfm_forest_field_entrance(room, 1);
    const uint8_t *box = bfm_forest_room_bytes(room, UINT32_C(0x80183ab8), 16);
    if (!descriptor || !destination || u16le(destination) != 0x300a || !box || u32le(box)) return 0;
    /* The upper doorway is an always-registered placement. Its floor trigger
     * appears only on the collision grid installed by the fourth switch. */
    uint32_t address = u32le(descriptor + 0xb4);
    for (;;) {
        const uint8_t *row = bfm_forest_room_bytes(room, address, 20);
        if (!row || !u16le(row + 6)) return 0;
        if (u16le(row + 6) == 126 && u16le(row + 8) == 2 &&
            bfm_forest_room_bytes(room, u32le(row + 16), 36) == destination) {
            if (u16le(row + 10) || u16le(row + 12)) return 0;
            BfmTowerAscentDoor found = {.entry = u16le(row + 14)};
            bfm_forest_decode_transfer(destination, &found.destination);
            for (unsigned axis = 0; axis < 3; ++axis) {
                found.position[axis] = s16le(row + 2 * axis);
                found.minimum[axis] = s16le(box + 4 + 4 * axis);
                found.maximum[axis] = s16le(box + 6 + 4 * axis);
                if (found.minimum[axis] > found.maximum[axis]) return 0;
            }
            *door = found;
            return 1;
        }
        address += 20;
    }
}

int bfm_tower_ascent_door_contact(const BfmTowerAscentDoor *door,
                                  const BfmForestContact *support, const int16_t feet[3])
{
    if (!door || !support || !feet || door->destination.room != 0x300a ||
        support->flags != 3) return 0;
    /* The caller supplies the floor the actor landed on. Selector 3 checks a
     * vertical span from three units above the feet to one unit below. */
    for (unsigned axis = 0; axis < 3; ++axis) {
        int local = feet[axis] - door->position[axis];
        int upper = local + (axis == 1);
        int lower = local - (axis == 1 ? 3 : 0);
        if (upper < door->minimum[axis] || lower > door->maximum[axis]) return 0;
    }
    return 1;
}
