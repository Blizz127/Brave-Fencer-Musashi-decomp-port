/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LENO_H
#define BFM_WOODS_LENO_H

#include "woods_leno_actor.h"
#include "bedroom_record.h"
#include "forest_message.h"
#include "village_ground.h"
#include "village_camera.h"

typedef struct BfmWoodsLeno BfmWoodsLeno;

typedef enum {
    BFM_LENO_NO_COMMAND, BFM_LENO_LIFT, BFM_LENO_THROW, BFM_LENO_DROP
} BfmWoodsLenoCommand;

typedef struct {
    const int16_t *player;          /* Actual player feet after movement. */
    int player_yaw;
    /* A socket refresh publishes position and facing. A release packet
     * supplies only wall-corrected position. NULL retains the prior root. */
    const int16_t *grip;
    BfmWoodsLenoCommand command;    /* Events from the player's carry action. */
    unsigned random;                /* Field random value for resting barks. */
} BfmWoodsLenoInput;

typedef struct {
    unsigned sound[2];
    unsigned dust, splash, water_impact;
    int16_t effect_xyz[3];
} BfmWoodsLenoEffects;

/* Borrows the mounted scene, save, collision and art for the room lifetime. */
BfmWoodsLeno *bfm_woods_leno_open(const uint8_t *scene, size_t bytes, BfmSaveRecord *save,
                                  const BfmVillageGround *ground, const BfmWoodsLenoActor *art);
void bfm_woods_leno_free(BfmWoodsLeno *dog);
const BfmWoodsLenoView *bfm_woods_leno_view(const BfmWoodsLeno *dog);
int bfm_woods_leno_held(const BfmWoodsLeno *dog);
int bfm_woods_leno_in_dialogue(const BfmWoodsLeno *dog);
/* The greeting has dispatched the player's gesture after the opening wait. */
int bfm_woods_leno_speaking(const BfmWoodsLeno *dog);
/* On an ordinary tick with no lift command, draw one field random value
 * only when the resting actor is about to finish its current motion. */
int bfm_woods_leno_random_due(const BfmWoodsLeno *dog);
/* Once per field camera update, after the actor and normal follow. Always
 * writes the presented view, keeping normal independent. Returns one on the
 * final restored frame, when follow may adopt its saved eye and target. */
int bfm_woods_leno_camera_step(BfmWoodsLeno *dog, const BfmVillageCamera *normal,
                               BfmVillageCamera *presented);

/* The ordinary movement contact and lift ray use the same mounted shape.
 * The player supplies the previous contact pose and eligible button edge. */
int bfm_woods_leno_brace(const BfmWoodsLeno *dog, const int16_t from[3], const int16_t to[3],
                         int16_t contact[3]);
int bfm_woods_leno_target(const BfmWoodsLeno *dog, const int16_t from[3], const int16_t to[3]);
/* A new charged hit queues Leno's recoil. The room supplies the hit cue
 * and displayed amount; a wave uses Leno's root for that presentation. */
int bfm_woods_leno_charged_blade(BfmWoodsLeno *dog, const int16_t from[3], const int16_t to[3],
                                 unsigned yaw, int16_t hit[3], int16_t normal[3]);
int bfm_woods_leno_charged_wave(BfmWoodsLeno *dog, const int16_t spine[3], unsigned yaw,
                                int16_t hit[3]);
int bfm_woods_leno_step(BfmWoodsLeno *dog, const BfmWoodsLenoInput *input,
                        BfmForestMessage **opened, BfmWoodsLenoEffects *effects);
/* The caller owns each opened message; after its CLOSED event free the cursor
 * in the normal message service and notify this actor once. */
int bfm_woods_leno_close_message(BfmWoodsLeno *dog);

#endif
