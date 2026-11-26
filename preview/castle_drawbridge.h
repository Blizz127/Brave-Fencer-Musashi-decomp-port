/* SPDX-License-Identifier: MIT */
#ifndef BFM_CASTLE_DRAWBRIDGE_H
#define BFM_CASTLE_DRAWBRIDGE_H

#include "castle_drawbridge_view.h"
#include "gameplay_run.h"
#include "room_descriptor.h"

typedef struct BfmCastleDrawbridge BfmCastleDrawbridge;
enum BfmDrawbridgeMotion {
    BFM_DRAWBRIDGE_STAND, BFM_DRAWBRIDGE_RUN, BFM_DRAWBRIDGE_JUMP
};

/* The caller keeps its existing run and actor across the room transition.
 * This event owns direction and the short jump until the castle gate opens. */
BfmCastleDrawbridge *bfm_castle_drawbridge_open(const MusashiDiscMedia *disc,
    const RoomTransferDescriptor *from_summit, const BfmGameplayRun *run);
void bfm_castle_drawbridge_free(BfmCastleDrawbridge *bridge);
int bfm_castle_drawbridge_tick(BfmCastleDrawbridge *bridge);
int bfm_castle_drawbridge_draw(BfmCastleDrawbridge *bridge,
    const BfmForestPose *pose, const BfmForestTransform *root, uint32_t visible_parts,
    uint32_t pixels[240][320]);
/* During the bridge transfer, the room is selected before it goes live. */
int bfm_castle_drawbridge_live(const BfmCastleDrawbridge *bridge);
unsigned bfm_castle_drawbridge_room(const BfmCastleDrawbridge *bridge);
int bfm_castle_drawbridge_descending(const BfmCastleDrawbridge *bridge);
int bfm_castle_drawbridge_actor(const BfmCastleDrawbridge *bridge, int16_t feet[3],
    int16_t *heading, enum BfmDrawbridgeMotion *motion, unsigned *motion_tick);
/* The preceding room still owns its visible departure frame during this fade. */
int bfm_castle_drawbridge_uses_departure(const BfmCastleDrawbridge *bridge);
unsigned bfm_castle_drawbridge_shade(const BfmCastleDrawbridge *bridge);
/* Read and consume the mounted exit once after the walk and final pause. */
int bfm_castle_drawbridge_take_boss_request(BfmCastleDrawbridge *bridge,
    RoomTransferDescriptor *destination);
#endif
