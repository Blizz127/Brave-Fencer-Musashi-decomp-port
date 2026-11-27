/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_BOX_VISIT_H
#define BFM_WOODS_BOX_VISIT_H

#include "bedroom_record.h"
#include "field_input.h"
#include "forest_message.h"
#include "gameplay_run.h"
#include "village_arrival_motion.h"
#include "retry_screen.h"

typedef struct BfmWoodsBoxVisit BfmWoodsBoxVisit;
struct BfmBoulderImpact;

/* Receive a physical strike on Musashi after the projectile actor has moved.
 * Its displacement and item restriction take effect now. The ordinary player
 * processes the attack on his next field update. */
int bfm_woods_box_visit_boulder_hit(BfmWoodsBoxVisit *visit, const struct BfmBoulderImpact *impact);

/* Mount the receiving 3026 field on an ordinary door or a recorded Box retry.
 * The live entry, resident player data and shared VRAM outlive this visit.
 * The room transaction restores VRAM if any part of the new mount fails. */
BfmWoodsBoxVisit *bfm_woods_box_visit_open(const MusashiDiscMedia *disc,
    BfmBedroomEntry *entry, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024]);
void bfm_woods_box_visit_free(BfmWoodsBoxVisit *visit);
const BfmVillagePlayer *bfm_woods_box_visit_player(const BfmWoodsBoxVisit *visit);
/* Non-null only after ordinary contact with the door back to Cleft Woods. */
const BfmVillageMotion *bfm_woods_box_visit_departure(const BfmWoodsBoxVisit *visit);
int bfm_woods_box_visit_step(BfmWoodsBoxVisit *visit, BfmGameplayRun *run,
                             BfmFieldInput input, BfmForestMessage **opened);
int bfm_woods_box_visit_step_at(BfmWoodsBoxVisit *visit, BfmGameplayRun *run,
                                BfmFieldInput input, unsigned actor_tick, BfmForestMessage **opened);
int bfm_woods_box_visit_close(BfmWoodsBoxVisit *visit, unsigned answer);
int bfm_woods_box_visit_take_hud(BfmWoodsBoxVisit *visit);
void bfm_woods_box_visit_score_applied(BfmWoodsBoxVisit *visit, unsigned selector);
unsigned bfm_woods_box_visit_score(const BfmWoodsBoxVisit *visit);
int bfm_woods_box_visit_take_retired_window(BfmWoodsBoxVisit *visit);
unsigned bfm_woods_box_visit_take_voice(BfmWoodsBoxVisit *visit);
unsigned bfm_woods_box_visit_take_sound(BfmWoodsBoxVisit *visit);
int bfm_woods_box_visit_begin_retry(BfmWoodsBoxVisit *visit, BfmGameplayRun *run);
BfmRetryScreen *bfm_woods_box_visit_retry(const BfmWoodsBoxVisit *visit);
int bfm_woods_box_visit_draw_retry(BfmWoodsBoxVisit *visit, const BfmForestHud *hud,
                                   uint32_t pixels[240][320]);
int bfm_woods_box_visit_prepare_recovery(const BfmWoodsBoxVisit *visit, const BfmGameplayRun *run,
                                        BfmBedroomEntry *out);
int bfm_woods_box_visit_draw(BfmWoodsBoxVisit *visit, const uint8_t flags[64],
                             const BfmForestMessageFrame *window, uint32_t pixels[240][320]);

#endif
