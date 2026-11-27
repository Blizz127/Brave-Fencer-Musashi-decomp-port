/* SPDX-License-Identifier: MIT */
#include "forest_maze_gate.h"
#include "../pc_port/forest_player_bytes.h"

static const uint8_t *row(const uint8_t *scene, size_t size, uint32_t address, size_t count)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (!scene || address < origin || count > size || address-origin > size-count)
        return NULL;
    return scene + (address-origin);
}

static const uint8_t *room_header(const uint8_t *scene, size_t size, uint32_t header,
                                  uint32_t expected)
{
    const uint8_t *slot = row(scene, size, header, 4);
    return slot && u32le(slot) == expected ? row(scene, size, expected, 0xcc) : NULL;
}

static const uint8_t *door(const uint8_t *scene, size_t size, const uint8_t *room,
                           unsigned entrance)
{
    size_t offset = 4 * entrance;
    const uint8_t *list = row(scene, size, u32le(room+0xc8), offset+4);
    return list ? row(scene, size, u32le(list+offset), 36) : NULL;
}

/* The maze has several fixed exits of the same class. Match the descriptor
 * as well as the class so another trail cannot return Musashi to the bank. */
static const uint8_t *gate_actor(const uint8_t *scene, size_t size, uint32_t address,
                                 unsigned type, unsigned variant, uint32_t target)
{
    const uint8_t *actor;
    while ((actor = row(scene, size, address, 20)) && u16le(actor+6)) {
        if (u16le(actor+6) == type && u16le(actor+8) == variant) {
            const uint8_t *destination = row(scene, size, u32le(actor+16), 36);
            if (destination && u32le(destination) == target) return actor;
        }
        if (address > UINT32_MAX-20) break;
        address += 20;
    }
    return NULL;
}

int bfm_forest_maze_bank_gate(BfmForestMazeGate *gate, const uint8_t *scene,
                              size_t size, unsigned entrance, unsigned story)
{
    if (!gate || entrance > 2) return 0;
    const uint8_t *room = room_header(scene, size, 0x8018c594, 0x801910e4);
    if (!room) return 0;
    const uint8_t *actor = gate_actor(scene, size, u32le(room+0xb4), 361, 1, 0x301b);
    const uint8_t *routes = row(scene, size, 0x80190894, 8);
    const uint8_t *bounds = row(scene, size, 0x801908a4, 16);
    const uint8_t *arrival = door(scene, size, room, entrance);
    if (!actor || !routes || !bounds || !arrival || u32le(routes) != u32le(actor+16)) return 0;
    unsigned later = story >= 750;
    const uint8_t *destination = row(scene, size, u32le(routes+4*later), 36);
    if (!destination || u32le(destination) != (later ? 0x21301bu : 0x301bu)) return 0;
    *gate = (BfmForestMazeGate){arrival, destination, actor, bounds};
    return 1;
}

int bfm_forest_maze_return_gate(BfmForestMazeGate *gate, const uint8_t *scene,
                                size_t size, unsigned entrance)
{
    if (!gate || (entrance != 0 && entrance != 33)) return 0;
    const uint8_t *room = room_header(scene, size, 0x8018738c, 0x8018c890);
    if (!room) return 0;
    const uint8_t *arrival = door(scene, size, room, entrance);
    const uint8_t *bounds = row(scene, size, 0x80187938, 16);
    if (!arrival || !bounds) return 0;

    /* The later bank route installs a different actor set on entry to the
     * same clearing. Its southern gate has its own transfer callbacks. */
    uint32_t actors = entrance == 33 ? UINT32_C(0x8018ce04) : u32le(room+0xb4);
    const uint8_t *actor = gate_actor(scene, size, actors, 126, entrance == 0, 0x23014);
    if (!actor) return 0;
    *gate = (BfmForestMazeGate){arrival, row(scene, size, u32le(actor+16), 36), actor, bounds};
    return 1;
}

const uint8_t *bfm_forest_maze_gate_contact(const BfmForestMazeGate *gate,
                                           unsigned wall_contact, const int16_t feet[3])
{
    if (!gate || !gate->bounds || !gate->actor || !feet || (wall_contact & 255u) != 3)
        return NULL;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int foot = feet[axis] - s16le(gate->actor + 2*axis);
        const uint8_t *extent = gate->bounds + 4 + 4*axis;
        int low = foot - (axis == 1 ? 3 : 0);
        int high = foot + (axis == 1 ? 1 : 0);
        if (high < s16le(extent) || low > s16le(extent+2)) return NULL;
    }
    return gate->destination;
}

void bfm_forest_maze_gate_depart(const BfmForestMazeGate *gate,
                                uint8_t flags[64], uint8_t variables[256])
{
    if (!gate || !gate->destination || !gate->actor || !flags || !variables) return;
    uint32_t target = u32le(gate->destination);
    if (target == 0x301b || target == 0x21301b) {
        flags[322/8] &= (uint8_t)~(1u << (322%8));
        if (target == 0x301b && variables[204] == 1) variables[204] = 2;
    } else if (target == 0x23014 && u16le(gate->actor+8) == 1 &&
               variables[204] > 0 && variables[204] < 15) {
        variables[204] = 1;
    }
}

unsigned bfm_forest_maze_gate_arrive(const BfmForestMazeGate *gate, uint8_t flags[64])
{
    /* Maze entrance 33 selects the alternate actor lists. Revisiting
     * that layout while its flag is still set plays the clearing cue. */
    if (!gate || !gate->actor || !flags ||
        u16le(gate->actor+6) != 126 || u16le(gate->actor+8) != 0) return 0;
    unsigned mask = 1u << (322%8);
    if (flags[322/8] & mask) return 0xc4d;
    flags[322/8] |= (uint8_t)mask;
    return 0;
}
