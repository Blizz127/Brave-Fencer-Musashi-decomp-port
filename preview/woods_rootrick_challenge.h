/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_CHALLENGE_H
#define BFM_WOODS_ROOTRICK_CHALLENGE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    BFM_ROOTRICK_HIDE_HUD, BFM_ROOTRICK_SHOW_HUD,
    BFM_ROOTRICK_WALK_PLAYER, BFM_ROOTRICK_FACE_POINT, BFM_ROOTRICK_FACE_ACTOR,
    BFM_ROOTRICK_PLAYER_NOTICE, BFM_ROOTRICK_CAMERA_SHOT,
    BFM_ROOTRICK_ACTOR_STAGE, BFM_ROOTRICK_BIND_POSE, BFM_ROOTRICK_SET_YAW,
    BFM_ROOTRICK_TURN_TO_PLAYER, BFM_ROOTRICK_ACTOR_MESSAGE,
    BFM_ROOTRICK_PLAYER_MESSAGE, BFM_ROOTRICK_RACE_CAMERA,
    BFM_ROOTRICK_WALK_FROM_PLAYER, BFM_ROOTRICK_WALK_FROM_ACTOR,
    BFM_ROOTRICK_PLAYER_YAW, BFM_ROOTRICK_PLAYER_RELATIVE_YAW,
    BFM_ROOTRICK_PLAYER_CONTROL_BITS, BFM_ROOTRICK_PLAYER_ATTACK,
    BFM_ROOTRICK_PLAYER_TARGET, BFM_ROOTRICK_TURN_ACTOR, BFM_ROOTRICK_DRIVE_ACTOR,
    BFM_ROOTRICK_ACTOR_VISIBLE, BFM_ROOTRICK_SUMMIT_BOULDER,
    BFM_ROOTRICK_STAGE_SCORE, BFM_ROOTRICK_ROOM_CAMERA
} BfmRootrickEvent;

typedef struct {
    BfmRootrickEvent event;
    /* Walk/point coordinates are signed X/Z halfwords (low/high).
     * Clip addresses select mounted actor-bank data and restart that clip. */
    uint32_t operand;
    /* A shot borrows two mounted vectors; a message borrows its field stream. */
    const uint8_t *data;
    size_t bytes;
    int window_offset;
    unsigned window_flags;
} BfmRootrickCommand;

typedef int (*BfmRootrickApply)(void *context, const BfmRootrickCommand *command);
typedef enum {
    BFM_ROOTRICK_ERROR, BFM_ROOTRICK_RUNNING, BFM_ROOTRICK_FINISHED
} BfmRootrickResult;

typedef struct {
    const uint8_t *scene;
    size_t bytes, cursor, end;
    uint32_t last_message;
    unsigned outcome, walk_from_player, waiting_score;
    unsigned delay, waiting_message, listening, terminal, failed;
    int offsets[2];
    unsigned window_flags[2];
} BfmRootrickChallenge;

/* Start only from the mounted Rootrick actor's challenge-start phase. The room
 * outlives the script and all its borrowers. Run one step on the entry field. */
int bfm_woods_rootrick_challenge_begin(BfmRootrickChallenge *script,
                                       const uint8_t *scene, size_t bytes);
/* One field per update. A room-actor stage may release a delay early. A real
 * message stays active until its window closes. The caller runs the actor's
 * choreography before polling and services the normal field-message flags. */
BfmRootrickResult bfm_woods_rootrick_challenge_step(BfmRootrickChallenge *script,
    int window_active, int actor_release, uint8_t flags[64], BfmRootrickApply apply, void *context);

/* Begin only with the mounted type-128 race controller's resolved outcome:
 * 1 = lost, 2 = won, 3 = cape ruse. Undecided cannot start a result script.
 * Step after actor update. score_ready is the real score worker's readiness.
 * The current room, flags and player remain live across the whole program. */
int bfm_woods_rootrick_outcome_begin(BfmRootrickChallenge *script,
    const uint8_t *scene, size_t bytes, unsigned earned_outcome);
BfmRootrickResult bfm_woods_rootrick_outcome_step(BfmRootrickChallenge *script,
    int window_active, int actor_release, int score_ready,
    uint8_t flags[64], BfmRootrickApply apply, void *context);

/* Once the actor has observed FINISHED and released the climbing ledges,
 * its separate retirement phase waits for the room score only on a loss. */
int bfm_woods_rootrick_outcome_can_retire(const BfmRootrickChallenge *script, int score_ready);

/* WALK_FROM_PLAYER adds the signed X/Z offsets in world axes to the player's
 * current position. WALK_FROM_ACTOR transforms local X/Z through Rootrick.
 * PLAYER_RELATIVE_YAW adds the angle to the player's current yaw. PLAYER_TARGET packs actor kind
 * and variant, low/high. PLAYER_ATTACK queues Square (0) or Triangle (1).
 * TURN_ACTOR packs signed relative angle and interpolation divisor, low/high.
 * DRIVE_ACTOR packs signed Q8 forward speed and field count, low/high.
 * SUMMIT_BOULDER 0 aligns Rootrick's X with the hero, then spawns type 186/0;
 * 1 aligns the hero's X with Rootrick, then 186/1 adopts the hero's full position.
 * STAGE_SCORE hands the selection to the original room's deferred worker.
 * ROOM_CAMERA restores normal tracking. No room or shrine transfer is implied. */
#endif
