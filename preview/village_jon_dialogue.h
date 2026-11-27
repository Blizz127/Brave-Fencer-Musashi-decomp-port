/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_JON_DIALOGUE_H
#define BFM_VILLAGE_JON_DIALOGUE_H

#include "bedroom_record.h"
#include "forest_message.h"
#include "village_jon_stage.h"

typedef struct {
    const uint8_t *module;
    size_t bytes;
    uint32_t cursor;
    unsigned phase, delay, answer, speaker;
    int first_visit, waiting, changed, send_dog;
} BfmVillageJonDialogue;

typedef enum {
    BFM_JON_DIALOGUE_ERROR = -1,
    BFM_JON_DIALOGUE_WAIT,
    BFM_JON_DIALOGUE_OPENED,
    BFM_JON_DIALOGUE_RETURN_CAMERA,
    BFM_JON_DIALOGUE_RELEASE
} BfmVillageJonDialogueEvent;

int bfm_village_jon_dialogue_eligible(const BfmSaveRecord *save);
/* The room keeps the decoded stocks event mounted through camera release. */
int bfm_village_jon_dialogue_begin(BfmVillageJonDialogue *talk, const uint8_t *module,
                                   size_t bytes, BfmSaveRecord *save);
void bfm_village_jon_dialogue_close(BfmVillageJonDialogue *talk, unsigned answer);
BfmVillageJonDialogueEvent bfm_village_jon_dialogue_step(BfmVillageJonDialogue *talk,
    BfmVillageJonStage *stage, int camera_restored, BfmSaveRecord *save,
    BfmForestMessage **opened);
#endif
