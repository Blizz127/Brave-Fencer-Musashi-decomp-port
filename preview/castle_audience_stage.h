/* SPDX-License-Identifier: MIT */
#ifndef BFM_CASTLE_AUDIENCE_STAGE_H
#define BFM_CASTLE_AUDIENCE_STAGE_H

#include "musashi_disc_media.h"
#include <stddef.h>
#include <stdint.h>

typedef struct BfmCastleAudienceStage BfmCastleAudienceStage;

/* SC01/9, the player's MAIN/10 bank and the shared room tiles are borrowed.
 * Install the MAIN/10 tiles followed by SC01/9 before opening. */
BfmCastleAudienceStage *bfm_castle_audience_stage_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes,
    uint16_t tiles[512][1024], unsigned equipped_orb);
void bfm_castle_audience_stage_free(BfmCastleAudienceStage *stage);
int bfm_castle_audience_stage_step(BfmCastleAudienceStage *stage);
/* Snapshot the two screen-space subject offsets when the actors set up their windows. */
int bfm_castle_audience_stage_speakers(const BfmCastleAudienceStage *stage,
                                     int *musashi_x, int *ribson_x);
void bfm_castle_audience_stage_begin_exit(BfmCastleAudienceStage *stage);
/* Render into the room page before submitting its HUD and conversation. */
int bfm_castle_audience_stage_draw(BfmCastleAudienceStage *stage, const uint8_t flags[64],
                                   uint32_t pixels[240][320]);
#endif
