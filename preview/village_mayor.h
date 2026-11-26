/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MAYOR_H
#define BFM_VILLAGE_MAYOR_H
#include "bedroom_record.h"
#include "forest_message.h"

typedef struct {
    const uint8_t *module;
    size_t bytes;
    uint32_t cursor;
    unsigned branch, delay, answer, hero, waiting, finished;
} BfmVillageMayorDialogue;

/* Begin when the door has staged both speakers. Keep the event member mounted
 * until the last field window closes. */
int bfm_village_mayor_dialogue_begin(BfmVillageMayorDialogue *talk, const uint8_t *module,
                                     size_t bytes, const BfmSaveRecord *save);
int bfm_village_mayor_dialogue_step(BfmVillageMayorDialogue *talk, BfmSaveRecord *save,
                                    BfmForestMessage **opened);
void bfm_village_mayor_dialogue_close(BfmVillageMayorDialogue *talk, unsigned answer);
#endif
