/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_SHAFT_GATES_H
#define BFM_VILLAGE_MINE_SHAFT_GATES_H

#include <stddef.h>
#include <stdint.h>

/* Borrowed class-126 placements and their associated transfer descriptors. */
typedef struct {
    struct {
        const uint8_t *actor, *door;
    } gate[4];
    const uint8_t *bounds;
} BfmVillageMineShaftGates;

int bfm_village_mine_shaft_gates_open(BfmVillageMineShaftGates *gates,
    const uint8_t *scene, size_t bytes, uint32_t header);
const uint8_t *bfm_village_mine_shaft_contact(const BfmVillageMineShaftGates *gates,
    const int16_t foot[3], unsigned wall_contact);
const uint8_t *bfm_village_mine_shaft_actor(const BfmVillageMineShaftGates *gates,
    const uint8_t *door);

#endif
