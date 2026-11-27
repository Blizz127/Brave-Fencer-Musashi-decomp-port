/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_GATE_TEAM_H
#define BFM_VALLEY_GATE_TEAM_H

#include "forest_message.h"
#include "gameplay_run.h"

/* Slots follow the four placed children of the gate controller. A child clears
 * its own WALK_TO_SWITCH request as it starts moving. */
typedef enum {
    BFM_VALLEY_GATE_SOLDIER, BFM_VALLEY_GATE_CARPENTER,
    BFM_VALLEY_GATE_MERCENARY, BFM_VALLEY_GATE_KNIGHT,
    BFM_VALLEY_GATE_MEMBER_COUNT
} BfmValleyGateMember;

enum { BFM_VALLEY_GATE_IDLE, BFM_VALLEY_GATE_SPEAK, BFM_VALLEY_GATE_WALK_TO_SWITCH };

typedef struct {
    const uint8_t *scene, *cursor, *end;
    uint8_t member[BFM_VALLEY_GATE_MEMBER_COUNT];
    unsigned members_present, gate_open_on_entry;
    unsigned face_point, window, window_right;
    int16_t face_x, face_z, window_offset;
    unsigned phase, delay, pending_signal;
    int16_t actor_offset;
    unsigned actor_right;
} BfmValleyGateTeam;

typedef enum {
    BFM_VALLEY_GATE_ERROR = -1, BFM_VALLEY_GATE_FREE,
    BFM_VALLEY_GATE_LOCKED, BFM_VALLEY_GATE_RELEASED
} BfmValleyGateResult;

/* The caller keeps decoded SC03/31 and the live save mounted for this visit. */
int bfm_valley_gate_team_init(BfmValleyGateTeam *team, const uint8_t *scene, size_t bytes,
                              const BfmSaveRecord *save);
int bfm_valley_gate_team_active(const BfmValleyGateTeam *team);
/* Tick even while a page is focused. hero_z is the signed world coordinate.
 * player_action_ready reports that the scripted player action has settled.
 * A face_point result queues a turn toward (face_x, face_z) on this update.
 * LOCKED owns ordinary input. RELEASED publishes story and releases the hero.
 * The caller owns each opened page and services its normal field voice. */
BfmValleyGateResult bfm_valley_gate_team_step(BfmValleyGateTeam *team, int hero_z,
    int player_action_ready, BfmSaveRecord *save, BfmGameplayRun *run, BfmForestMessage **opened);
/* Report closure only after the shared field window and its voice have run. */
int bfm_valley_gate_team_message_closed(BfmValleyGateTeam *team);

#endif
