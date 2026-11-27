/* SPDX-License-Identifier: MIT */
#ifndef BFM_SOUTHERN_WOODS_H
#define BFM_SOUTHERN_WOODS_H

#include "bedroom_record.h"
#include "field_input.h"
#include "gameplay_run.h"
#include "forest_message.h"
#include "village_arrival_motion.h"
#include "retry_screen.h"

typedef struct BfmSouthernWoods BfmSouthernWoods;

BfmSouthernWoods *bfm_southern_woods_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
    const uint8_t *door, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024]);
void bfm_southern_woods_free(BfmSouthernWoods *woods);
/* The rescue field first animates during the entry fade, before hero updates resume. */
void bfm_southern_woods_rescue_field_step(BfmSouthernWoods *woods, BfmGameplayRun *run,
                                          unsigned field_tick);
/* A negative value means no unbroken rescue field is registered. */
int bfm_southern_woods_detector(const BfmSouthernWoods *woods);
int bfm_southern_woods_step_field(BfmSouthernWoods *woods, BfmGameplayRun *run, BfmFieldInput field,
                                  BfmForestMessage **opened);
int bfm_southern_woods_step_field_at(BfmSouthernWoods *woods, BfmGameplayRun *run,
                                     BfmFieldInput field, unsigned actor_tick, BfmForestMessage **opened);
int bfm_southern_woods_award_cue(const BfmSouthernWoods *woods);
unsigned bfm_southern_woods_sound(const BfmSouthernWoods *woods);
size_t bfm_southern_woods_take_sounds(BfmSouthernWoods *woods, unsigned *out, size_t capacity);
int bfm_southern_woods_tether_gauge(const BfmSouthernWoods *woods);
int bfm_southern_woods_step(BfmSouthernWoods *woods, int x, int y,
                            int talk_edge, BfmForestMessage **opened);
int bfm_southern_woods_close(BfmSouthernWoods *woods);
/* A fatal hit is offered to the field's defeat handler once, after checking
 * the working inventory for S-Revive. */
int bfm_southern_woods_take_zero_hp(BfmSouthernWoods *woods);
/* Begin only on an earned, unstocked fatal reaction. */
int bfm_southern_woods_begin_retry(BfmSouthernWoods *woods, BfmGameplayRun *run);
BfmRetryScreen *bfm_southern_woods_retry(const BfmSouthernWoods *woods);
int bfm_southern_woods_prepare_recovery(const BfmSouthernWoods *woods, const BfmGameplayRun *run,
                                       BfmBedroomEntry *out);
int bfm_southern_woods_draw_retry(BfmSouthernWoods *woods, const BfmForestHud *hud,
                                  uint32_t pixels[240][320]);
int bfm_southern_woods_carrying_leno(const BfmSouthernWoods *woods);
const BfmVillageMotion *bfm_southern_woods_departure(const BfmSouthernWoods *woods);
const BfmVillagePlayer *bfm_southern_woods_player(const BfmSouthernWoods *woods);
int bfm_southern_woods_draw(BfmSouthernWoods *woods, const uint8_t flags[64],
                            const BfmForestMessageFrame *window, uint32_t pixels[240][320]);

#endif
