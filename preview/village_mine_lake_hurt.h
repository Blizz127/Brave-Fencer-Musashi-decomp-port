/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_HURT_H
#define BFM_VILLAGE_MINE_LAKE_HURT_H

#include "village_field_defeat.h"
#include "village_switchback_hit.h"

typedef struct BfmMineLakeHurt BfmMineLakeHurt;

BfmMineLakeHurt *bfm_mine_lake_hurt_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, BfmVillageMotion *motion, BfmSaveRecord *save);
void bfm_mine_lake_hurt_free(BfmMineLakeHurt *hurt);
/* The player receives the last occupied actor's hit on its following field. */
void bfm_mine_lake_hurt_queue(BfmMineLakeHurt *hurt, const BfmMineLungeHit *incoming);
/* Call after the preceding current is installed and before ordinary motion.
 * Negative means failure, zero permits ordinary movement, positive consumes it. */
int bfm_mine_lake_hurt_step(BfmMineLakeHurt *hurt, BfmGameplayRun *run, BfmFieldInput input,
    unsigned camera_yaw, unsigned field);
/* Call once per actor pass after the completed player foot has been published. */
const int16_t *bfm_mine_lake_hurt_target(BfmMineLakeHurt *hurt);
int bfm_mine_lake_hurt_body(const BfmMineLakeHurt *hurt, const uint8_t *main, size_t bytes,
    BfmBedroomPose *pose, unsigned *visible);
int bfm_mine_lake_hurt_draw(const BfmMineLakeHurt *hurt, BfmBedroomFigure *figure,
    const BfmBedroomView *view);
unsigned bfm_mine_lake_hurt_take_sound(BfmMineLakeHurt *hurt);
int bfm_mine_lake_hurt_defeated(const BfmMineLakeHurt *hurt);

typedef enum {
    BFM_MINE_LAKE_LIFE_ERROR = -1,
    BFM_MINE_LAKE_REVIVED,
    BFM_MINE_LAKE_RETRY
} BfmMineLakeLife;

/* The renderer context borrows the active room and must outlive the chooser. */
BfmMineLakeLife bfm_mine_lake_hurt_resolve(BfmMineLakeHurt *hurt, BfmGameplayRun *run,
    BfmVillageFieldDefeat *actor, const BfmVillageCamera *presented,
    int item_blocked, BfmForestMessage **notice);
BfmRetryScreen *bfm_mine_lake_hurt_retry(const BfmMineLakeHurt *hurt);
int bfm_mine_lake_hurt_restore(const BfmMineLakeHurt *hurt, const BfmGameplayRun *run,
    BfmSaveRecord *out);
#endif
