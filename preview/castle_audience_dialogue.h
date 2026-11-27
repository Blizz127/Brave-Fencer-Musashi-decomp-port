/* SPDX-License-Identifier: MIT */
#ifndef BFM_CASTLE_AUDIENCE_DIALOGUE_H
#define BFM_CASTLE_AUDIENCE_DIALOGUE_H

#include "forest_message.h"
#include "gameplay_run.h"

typedef struct {
    const uint8_t *scene;
    size_t bytes;
    unsigned phase, row, helpers, wait;
    int hero;
} BfmRibsonDialogue;

typedef enum {
    BFM_RIBSON_INVALID, BFM_RIBSON_WAIT, BFM_RIBSON_PAGE,
    BFM_RIBSON_SERVICE, BFM_RIBSON_DEPART
} BfmRibsonResult;

int bfm_castle_audience_eligible(const BfmSaveRecord *record);
void bfm_ribson_dialogue_begin(BfmRibsonDialogue *talk, const uint8_t *scene, size_t bytes);
/* Called on a live court update only when the previous message no longer owns focus. */
BfmRibsonResult bfm_ribson_dialogue_step(BfmRibsonDialogue *talk, BfmSaveRecord *save,
                                        BfmGameplayRun *run, BfmForestMessage **out);
#endif
