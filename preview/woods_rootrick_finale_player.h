/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_FINALE_PLAYER_H
#define BFM_WOODS_ROOTRICK_FINALE_PLAYER_H

#include "woods_rootrick_challenge.h"
#include "village_arrival_motion.h"
#include "woods_carry.h"
#include "forest_guard_contact.h"
#include "woods_caster_melee.h"

typedef struct BfmRootrickFinalePlayer BfmRootrickFinalePlayer;

typedef struct {
    BfmWoodsCarryFrame carry;
    unsigned socket_yaw, socket_turn; /* Keep the rock's current yaw unless socket_turn. */
    BfmWoodsCasterMeleeFrame blade;
    int ordinary_standing; /* This scripted player field actually ran grounded standing. */
    uint32_t control_bits; /* New grants from this field's accepted control command. */
    uint32_t granted_control_bits; /* All grants through this field, until normal release. */
} BfmRootrickPlayerFrame;

/* Borrow the mounted Twinpeak room, resident player bank and live motion. */
BfmRootrickFinalePlayer *bfm_woods_rootrick_player_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    BfmVillageMotion *motion);
void bfm_woods_rootrick_player_free(BfmRootrickFinalePlayer *player);
int bfm_woods_rootrick_player_begin(BfmRootrickFinalePlayer *player, unsigned outcome);

/* Winning boulder spawn aligns only the live hero X to Rootrick's complete
 * Q16 coordinate. Call in the spawn callback before acquiring the rock. */
int bfm_woods_rootrick_player_align_x(BfmRootrickFinalePlayer *player, int32_t x_q16);
/* Issue during the actor's script update. Resolve relative positions using
 * the two live actors now. Returns 1 for accepted, 0 for another receiver
 * and -1 for invalid data. The boulder owner applies its immediate X alignment. */
int bfm_woods_rootrick_player_queue(BfmRootrickFinalePlayer *player,
    const BfmRootrickCommand *command, const BfmForestTransform *rival);

/* Once per player field while the outcome owns Musashi. A winning rock must
 * exist before target acquisition. Forward attached sockets and the marked
 * throw to that rock. Test real blade rays against the live rival. A loss
 * publishes the new control bits and leaves its hurt/defeat body with the
 * field's damage owner. camera_yaw uses the village field convention. */
int bfm_woods_rootrick_player_step(BfmRootrickFinalePlayer *player, BfmGameplayRun *run,
    int camera_yaw, uint32_t actor_flags, int winning_rock_present,
    BfmRootrickPlayerFrame *frame);
/* Call while the rival can receive a blade. Clip each active weapon ray to
 * the room and test his first mounted body against its previous model root.
 * A miss preserves hit and normal. */
int bfm_woods_rootrick_player_rival_hit(const BfmRootrickFinalePlayer *player,
    const BfmWoodsCasterMeleeFrame *blade, const BfmForestGuardBox *body,
    const BfmForestTransform *rival, int16_t hit[3], int16_t normal[3]);
/* Room script terminator, after its actor released the outcome. A loss with
 * defeat in progress keeps the regular hurt/game-over owner and its pose. */
int bfm_woods_rootrick_player_finish(BfmRootrickFinalePlayer *player, int defeating);
#endif
