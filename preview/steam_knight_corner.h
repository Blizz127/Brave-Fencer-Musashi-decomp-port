/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_CORNER_H
#define BFM_STEAM_KNIGHT_CORNER_H

#include "forest_field.h"
#include "forest_player_state.h"

typedef struct {
    enum {
        BFM_CORNER_UNUSED, BFM_CORNER_ENTRY, BFM_CORNER_DISPATCH, BFM_CORNER_START_WALK,
        BFM_CORNER_WALKING, BFM_CORNER_WAIT_VIEW, BFM_CORNER_VIEW_READY, BFM_CORNER_DONE
    } phase;
    int16_t target[3];
} BfmSteamCorner;

typedef enum {
    BFM_CORNER_BLOCKED = -1, BFM_CORNER_IDLE, BFM_CORNER_WALK,
    BFM_CORNER_AT_MARK, BFM_CORNER_CONTROLS
} BfmSteamCornerStep;

/* Begin once on the street's marked western jump, after the outgoing player
 * update. The encounter supplies the player waypoint. Step on this same
 * update to cancel the previous action at its actual fractional position. */
int bfm_steam_corner_begin(BfmSteamCorner *corner, const int16_t target[3]);

/* Apply one field-player update. The actor and its integer feet belong to the
 * same player. AT_MARK requests the western camera once. CONTROLS lets the
 * caller run normal controls on this update. west_view_settled reflects the
 * preceding camera update. No player buttons are consumed while walking. */
BfmSteamCornerStep bfm_steam_corner_step(BfmSteamCorner *corner, const BfmForestField *field,
    ForestPlayerState *actor, int16_t feet[3], int16_t *yaw, int camera_yaw, int west_view_settled);
#endif
