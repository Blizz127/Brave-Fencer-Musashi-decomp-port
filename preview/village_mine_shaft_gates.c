/* SPDX-License-Identifier: MIT */
#include "village_mine_shaft_gates.h"
#include "../pc_port/forest_player_bytes.h"

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t need)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base) return NULL;
    size_t offset = address - base;
    return offset <= bytes && need <= bytes - offset ? scene + offset : NULL;
}

int bfm_village_mine_shaft_gates_open(BfmVillageMineShaftGates *gates,
    const uint8_t *scene, size_t bytes, uint32_t header)
{
    if (!gates || !scene || header != UINT32_C(0x80181f28)) return 0;
    const uint8_t *room = mounted(scene, bytes, header, 0xcc);
    if (!room) return 0;
    BfmVillageMineShaftGates next = {0};
    const uint8_t *actors = mounted(scene, bytes, u32le(room + 0xb4), 4 * 20);
    const uint8_t *doors = mounted(scene, bytes, u32le(room + 0xc8), 4 * 4);
    const uint8_t *shape = mounted(scene, bytes, UINT32_C(0x8017e8ac), 16);
    if (!actors || !doors || !shape || u32le(shape)) return 0;
    next.bounds = shape + 4;
    for (unsigned i = 0; i < 4; ++i) {
        const uint8_t *actor = actors + 20*i;
        uint32_t destination = u32le(doors + 4*i);
        if (u16le(actor + 6) != 126 || u32le(actor + 16) != destination) return 0;
        next.gate[i].actor = actor;
        next.gate[i].door = mounted(scene, bytes, destination, 36);
        if (!next.gate[i].door) return 0;
    }
    *gates = next;
    return 1;
}

const uint8_t *bfm_village_mine_shaft_contact(const BfmVillageMineShaftGates *gates,
    const int16_t foot[3], unsigned wall_contact)
{
    if (!gates || !gates->bounds || !foot || (wall_contact & 0xff) != 3) return NULL;
    const uint8_t *box = gates->bounds;
    for (unsigned i = 0; i < 4; ++i) {
        const uint8_t *actor = gates->gate[i].actor;
        int x = foot[0] - s16le(actor);
        int y = foot[1] - s16le(actor + 2);
        int z = foot[2] - s16le(actor + 4);
        /* The actor service tests the short vertical segment through the foot. */
        if (x >= s16le(box) && x <= s16le(box + 2) &&
            y + 1 >= s16le(box + 4) && y - 3 <= s16le(box + 6) &&
            z >= s16le(box + 8) && z <= s16le(box + 10))
            return gates->gate[i].door;
    }
    return NULL;
}

const uint8_t *bfm_village_mine_shaft_actor(const BfmVillageMineShaftGates *gates,
    const uint8_t *door)
{
    if (!gates || !door) return NULL;
    for (unsigned i = 0; i < 4; ++i)
        if (door == gates->gate[i].door) return gates->gate[i].actor;
    return NULL;
}
