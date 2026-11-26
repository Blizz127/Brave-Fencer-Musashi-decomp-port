/* SPDX-License-Identifier: MIT */
#ifndef BFM_CASTLE_DRAWBRIDGE_VIEW_H
#define BFM_CASTLE_DRAWBRIDGE_VIEW_H

#include "forest_field.h"

typedef struct BfmCastleDrawbridgeView BfmCastleDrawbridgeView;

BfmCastleDrawbridgeView *bfm_castle_drawbridge_view_open(const MusashiDiscMedia *disc);
void bfm_castle_drawbridge_view_free(BfmCastleDrawbridgeView *view);
/* The view owns the room. Its two mounted entries are the scripted arrival
 * in front of the bridge and the subsequent castle transfer. */
BfmForestField *bfm_castle_drawbridge_view_room(BfmCastleDrawbridgeView *view);

/* Room updates start when the castle entrance view is revealed. The event
 * controller supplies Musashi's model transform, pose and visible parts. */
int bfm_castle_drawbridge_view_draw(BfmCastleDrawbridgeView *view, unsigned room_tick,
                                    const BfmForestPose *pose,
                                    const BfmForestTransform *root, uint32_t visible_parts,
                                    uint32_t pixels[240][320]);

#endif
