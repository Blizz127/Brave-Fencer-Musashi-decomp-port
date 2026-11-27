/* SPDX-License-Identifier: MIT */
#ifndef BFM_CASTLE_AUDIENCE_VISITORS_H
#define BFM_CASTLE_AUDIENCE_VISITORS_H

#include "bedroom_record.h"
#include "forest_message_art.h"

typedef struct BfmCastleMenuArt BfmCastleMenuArt;

typedef enum {
    BFM_CASTLE_VISIT_ERROR, BFM_CASTLE_VISIT_ACTIVE,
    BFM_CASTLE_VISIT_FAREWELL, BFM_CASTLE_VISIT_GUEST
} BfmCastleVisitResult;

typedef struct {
    const uint8_t *scene, *main;
    size_t bytes, main_bytes;
    BfmCastleMenuArt *menu_art;
    unsigned phase, answer, menu_row, guest_row, scroll, guest, ink;
    unsigned prompt, story, clock, repeat, sound;
    uint8_t known_region[8];
    uint8_t availability[40];
    int waiting, hero, held;
} BfmCastleAudienceVisitors;

/* Begin at Ribson's follow-up question; the caller retains the scene for
 * the whole visit, ticks each opened window and supplies the answer on close. */
int bfm_castle_audience_visitors_begin(BfmCastleAudienceVisitors *visit,
    const MusashiDiscMedia *disc, const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes);
void bfm_castle_audience_visitors_free(BfmCastleAudienceVisitors *visit);
BfmCastleVisitResult bfm_castle_audience_visitors_step(BfmCastleAudienceVisitors *visit,
    const BfmSaveRecord *save, int direction, int held, int accept, int back, BfmForestMessage **opened);
void bfm_castle_audience_visitors_closed(BfmCastleAudienceVisitors *visit, unsigned answer);
int bfm_castle_audience_visitors_draw(const BfmCastleAudienceVisitors *visit,
    const BfmForestMessageArt *message_art, const uint16_t tiles[512][1024],
    uint32_t pixels[240][320]);
/* Resume Ribson's question when a summoned guest hands back control. */
void bfm_castle_audience_visitors_return(BfmCastleAudienceVisitors *visit);
#endif
