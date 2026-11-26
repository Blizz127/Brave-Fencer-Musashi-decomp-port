/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_ENTRY_H
#define BFM_FOREST_ENTRY_H

#include "forest_field.h"
#include "room_descriptor.h"

/* Decode one complete, bounds-checked transfer record from a mounted room. */
void bfm_forest_decode_transfer(const uint8_t record[36], RoomTransferDescriptor *out);

/* Resolve the transfer requested by Cross in the active opening scene. Pass
 * the decoded first PAC section of the source member from the mounted disc. */
int bfm_forest_entry_from_scene(const uint8_t *scene, size_t bytes, unsigned member,
                                RoomTransferDescriptor *out);

/* Load the room selected by the outgoing scene, then apply that room's own
 * selected entrance. The old player survives a failed load unmodified. */
BfmForestField *bfm_forest_enter(const MusashiDiscMedia *disc, const RoomTransferDescriptor *entry,
                                 ForestPlayerState *player, int16_t *model_yaw, int16_t cached_position[3]);
#endif
