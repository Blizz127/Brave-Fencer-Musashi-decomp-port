/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_ENCOUNTER_H
#define BFM_WOODS_ROOTRICK_ENCOUNTER_H

#include "woods_box_view.h"
#include "village_arrival_motion.h"
#include "forest_message_art.h"
#include "gameplay_run.h"

typedef struct BfmWoodsRootrickEncounter BfmWoodsRootrickEncounter;

BfmWoodsRootrickEncounter *bfm_woods_rootrick_open(const MusashiDiscMedia *disc,
                                                  const uint8_t *scene, size_t bytes,
                                                  const uint8_t *main, size_t main_bytes);
void bfm_woods_rootrick_free(BfmWoodsRootrickEncounter *encounter);
int bfm_woods_rootrick_update_ledges(BfmWoodsRootrickEncounter *encounter, BfmGameplayRun *run);
int bfm_woods_rootrick_controls_player(const BfmWoodsRootrickEncounter *encounter);
/* Run after normal movement and contact, or in place of normal movement while
 * the encounter owns Musashi. The first crossing still advances the field clock. */
int bfm_woods_rootrick_step(BfmWoodsRootrickEncounter *encounter, BfmVillageMotion *motion,
                           BfmWoodsBoxView *view, BfmGameplayRun *run, BfmSaveRecord *record,
                           BfmFieldInput input, unsigned actor_tick, int hero_standing, BfmForestMessage **opened);
int bfm_woods_rootrick_draw(const BfmWoodsRootrickEncounter *encounter,
                           BfmBedroomFigure *figure, const BfmBedroomView *view);
int bfm_woods_rootrick_closed(BfmWoodsRootrickEncounter *encounter);
int bfm_woods_rootrick_window(const BfmWoodsRootrickEncounter *encounter,
                             const BfmForestMessageArt *art, const BfmForestMessageFrame *window,
                             uint32_t pixels[240][320]);
int bfm_woods_rootrick_take_hud(BfmWoodsRootrickEncounter *encounter);
void bfm_woods_rootrick_score_applied(BfmWoodsRootrickEncounter *encounter, unsigned selector);
struct BfmBoulderImpact;
uint32_t bfm_woods_rootrick_player_protection(const BfmWoodsRootrickEncounter *encounter);
int bfm_woods_rootrick_take_hurt_suppression(BfmWoodsRootrickEncounter *encounter);
int bfm_woods_rootrick_take_boulder_hit(BfmWoodsRootrickEncounter *encounter, struct BfmBoulderImpact *impact);
int bfm_woods_rootrick_take_retired_window(BfmWoodsRootrickEncounter *encounter);
unsigned bfm_woods_rootrick_take_voice(BfmWoodsRootrickEncounter *encounter);
unsigned bfm_woods_rootrick_take_sound(BfmWoodsRootrickEncounter *encounter);
unsigned bfm_woods_rootrick_score(const BfmWoodsRootrickEncounter *encounter);

#endif
