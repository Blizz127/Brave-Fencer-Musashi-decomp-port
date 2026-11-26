/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_ASCENT_DOOR_H
#define BFM_TOWER_ASCENT_DOOR_H
#include "forest_field.h"
#include "room_descriptor.h"

typedef struct {
    RoomTransferDescriptor destination;
    int16_t position[3], minimum[3], maximum[3];
    unsigned entry;
} BfmTowerAscentDoor;
int bfm_tower_ascent_door_load(BfmTowerAscentDoor *door, const BfmForestField *room);
int bfm_tower_ascent_door_contact(const BfmTowerAscentDoor *door,
                                  const BfmForestContact *support, const int16_t feet[3]);
#endif
