/* SPDX-License-Identifier: MIT */
#include "woods_pole_pickup.h"
#include "bedroom_effects.h"
#include "../pc_port/forest_player_bytes.h"

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t needed)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base) return NULL;
    size_t offset = address - base;
    return offset <= bytes && needed <= bytes - offset ? scene + offset : NULL;
}

int bfm_woods_pole_pickups_load(BfmWoodsPolePickups *out, const uint8_t *scene, size_t bytes)
{
    const uint8_t *room = mounted(scene, bytes, UINT32_C(0x801961e8), 0xbc);
    const uint8_t *amounts = mounted(scene, bytes, UINT32_C(0x801929a0), 4 * sizeof(uint16_t));
    if (!out || !room || !amounts) return 0;
    uint32_t actor = u32le(room + 0xb8);
    BfmWoodsPolePickups next = {0};
    unsigned count = 0;
    for (unsigned row = 0; row < 96; ++row, actor += 20) {
        const uint8_t *data = mounted(scene, bytes, actor, 20);
        if (!data) return 0;
        unsigned type = u16le(data + 6);
        if (!type) {
            if (count != BFM_WOODS_POLE_PICKUP_COUNT) return 0;
            *out = next;
            return 1;
        }
        if (type != 51) continue;
        unsigned variant = u16le(data + 8);
        if ((variant & ~3u) != 0x100 || count == BFM_WOODS_POLE_PICKUP_COUNT) return 0;
        BfmWoodsPolePickup *item = &next.items[count++];
        for (unsigned axis = 0; axis < 3; ++axis) item->xyz[axis] = s16le(data + 2 * axis);
        item->tier = variant & 3u;
        item->amount = u16le(amounts + 2 * item->tier);
        if (!item->amount) return 0;
    }
    return 0;
}

unsigned bfm_woods_pole_pickups_step(BfmWoodsPolePickups *pickups, BfmForestStatus *player,
                                    const int16_t feet[3], unsigned *popup_amount)
{
    if (!popup_amount) return 0;
    *popup_amount = 0;
    if (!pickups || !player || !feet) return 0;
    for (unsigned i = 0; i < BFM_WOODS_POLE_PICKUP_COUNT; ++i) {
        BfmWoodsPolePickup *item = &pickups->items[i];
        if (item->collected) continue;
        int64_t distance_squared = 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            int64_t delta = (int)feet[axis] - item->xyz[axis];
            distance_squared += delta * delta;
        }
        if (distance_squared >= 48 * 48) continue;
        unsigned maximum = u16le(player->record + 0x3e);
        unsigned current = u16le(player->record + 0x40) + item->amount;
        if (current > maximum) current = maximum;
        player->record[0x40] = (uint8_t)current;
        player->record[0x41] = (uint8_t)(current >> 8);
        item->collected = 1;
        *popup_amount = item->amount;
        return BFM_WOODS_PICKUP_SOUND;
    }
    return 0;
}
