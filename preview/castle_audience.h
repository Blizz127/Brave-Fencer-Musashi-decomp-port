/* SPDX-License-Identifier: MIT */
#ifndef BFM_CASTLE_AUDIENCE_H
#define BFM_CASTLE_AUDIENCE_H
#include "castle_audience_dialogue.h"
#include "forest_hud.h"
#include "bedroom_effects.h"
#include "forest_voice.h"

typedef struct BfmCastleAudience BfmCastleAudience;

BfmCastleAudience *bfm_castle_audience_open(const MusashiDiscMedia *disc, const BfmBedroomEntry *entry,
                                           const uint8_t *main, size_t main_bytes);
int bfm_castle_audience_step(BfmCastleAudience *court, BfmSaveRecord *save, BfmGameplayRun *run,
                             int confirm_edge, int reveal_edge, int direction, int held, int accept_edge, int back_edge);
int bfm_castle_audience_draw(BfmCastleAudience *court, const BfmForestHud *hud, const BfmForestStatus *status,
                             uint32_t pixels[240][320]);
const uint8_t *bfm_castle_audience_transfer(const BfmCastleAudience *court);
int bfm_castle_audience_guest_pending(const BfmCastleAudience *court);
BfmBedroomEffects *bfm_castle_audience_effects(BfmCastleAudience *court);
unsigned bfm_castle_audience_take_sound(BfmCastleAudience *court);
ForestVoice *bfm_castle_audience_voice(BfmCastleAudience *court, unsigned *generation);
void bfm_castle_audience_free(BfmCastleAudience *court);
#endif
