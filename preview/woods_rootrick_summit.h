/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_SUMMIT_H
#define BFM_WOODS_ROOTRICK_SUMMIT_H

#include "woods_rootrick_boulder.h"
#include "woods_rootrick_finale_actor.h"
#include "woods_rootrick_challenge.h"
#include "village_arrival_motion.h"

typedef struct BfmWoodsRootrickSummit BfmWoodsRootrickSummit;
typedef struct {
    int release, shake, remove_shadow, struck_hero;
    unsigned voice, sound;
    BfmBoulderImpact impact;
} BfmWoodsSummitUpdate;

/* Keep the earned crest and player body alive throughout all three outcomes. */
BfmWoodsRootrickSummit *bfm_woods_rootrick_summit_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    BfmVillageMotion *motion, BfmWoodsRootrickActor *rival, int visible, unsigned outcome);
void bfm_woods_rootrick_summit_free(BfmWoodsRootrickSummit *summit);
int bfm_woods_rootrick_summit_draw(const BfmWoodsRootrickSummit *summit, BfmBedroomFigure *figure,
                                   const BfmBedroomView *view);
/* One field before the active result script polls. */
int bfm_woods_rootrick_summit_step(BfmWoodsRootrickSummit *summit, BfmGameplayRun *run,
    int camera_yaw, int standing, BfmWoodsSummitUpdate *update);
int bfm_woods_rootrick_summit_command(BfmWoodsRootrickSummit *summit,
                                      const BfmRootrickCommand *command);
int bfm_woods_rootrick_summit_visible(const BfmWoodsRootrickSummit *summit);
uint32_t bfm_woods_rootrick_summit_grants(const BfmWoodsRootrickSummit *summit);
int bfm_woods_rootrick_summit_finish(BfmWoodsRootrickSummit *summit, int defeat);
#endif
