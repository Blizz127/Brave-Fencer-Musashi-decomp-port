/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BANK_HOPPER_H
#define BFM_FOREST_BANK_HOPPER_H
#include "bedroom_figure.h"
#include "bedroom_travel.h"
#include "gameplay_run.h"

typedef struct BfmBankHoppers BfmBankHoppers;
BfmBankHoppers *bfm_bank_hoppers_open(const uint8_t *scene, size_t bytes);
void bfm_bank_hoppers_free(BfmBankHoppers *hoppers);
/* A positive return is the ordinary contact power. The field receiver owns
 * defense, invulnerability and the eventual HP debit. */
unsigned bfm_bank_hoppers_step(BfmBankHoppers *hoppers, const BfmBedroomRoot *player,
                               BfmGameplayRun *run, unsigned *attack_yaw);
/* A null aim point leaves a resting Hopper asleep even with the player's feet nearby. */
unsigned bfm_bank_hoppers_step_target(BfmBankHoppers *h, const BfmBedroomRoot *player,
                                      const int16_t aim_point[3], BfmGameplayRun *run,
                                      unsigned *attack_yaw);
int bfm_bank_hoppers_draw(const BfmBankHoppers *hoppers, BfmBedroomFigure *figure,
                          const BfmBedroomView *view);
/* Placement IDs follow the mounted roster (1..4). Bound actors keep an aim
 * center but accept no second blade contact or ordinary attack. */
unsigned bfm_bank_hoppers_generation(const BfmBankHoppers *h, int id);
int bfm_bank_hoppers_target(const BfmBankHoppers *h, int id, int16_t center[3]);
int bfm_bank_hoppers_root(const BfmBankHoppers *h, int id, int16_t xyz[3]);
int bfm_bank_hoppers_contact(const BfmBankHoppers *h, const int16_t from[3],
                             const int16_t to[3], int16_t hit[3]);
int bfm_bank_hoppers_latch(BfmBankHoppers *h, int id);
void bfm_bank_hoppers_meter(BfmBankHoppers *h, int id, unsigned meter);
void bfm_bank_hoppers_release(BfmBankHoppers *h, int id);
int bfm_bank_hoppers_absorb(BfmBankHoppers *h, int id);
#endif
