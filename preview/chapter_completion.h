/* SPDX-License-Identifier: MIT */
#ifndef BFM_CHAPTER_COMPLETION_H
#define BFM_CHAPTER_COMPLETION_H
#include "bedroom.h"
#include "inn_profile.h"

typedef enum {
    BFM_CHAPTER_INACTIVE, BFM_CHAPTER_SAVE_PROMPT, BFM_CHAPTER_SAVE_READING,
    BFM_CHAPTER_SAVE_FILES, BFM_CHAPTER_SAVE_CONFIRM, BFM_CHAPTER_SAVE_WRITING,
    BFM_CHAPTER_SAVED, BFM_CHAPTER_RESUME, BFM_CHAPTER_SAVE_ERROR,
    BFM_CHAPTER_TWO_TITLE, BFM_CHAPTER_BEDROOM, BFM_CHAPTER_COMPLETION_FINISHED
} BfmChapterSavePage;

typedef struct {
    BfmSaveMenu files;
    BfmChapterSavePage page;
    const char *issue;
    unsigned fields;
    BfmSaveRecord earned;
    BfmGameplayRun *run;
} BfmChapterCompletion;

/* Receive a completed encounter once. The caller retains its live run. Supply
 * its full saved state after selecting story two and before entering the
 * bedroom. Zero-initialize the receiver before first use. */
int bfm_chapter_completion_begin(BfmChapterCompletion *state, BfmGameplayRun *run,
                                 const BfmSaveRecord *record);
/* One NTSC field with fresh wire-order edges. Zero leaves an unimplemented
 * choice with the caller. The recorded branch accepted both Yes prompts. */
int bfm_chapter_completion_step(BfmChapterCompletion *state, const MusashiDiscMedia *disc,
                                BfmInnProfile *copy, uint16_t pressed);
/* Transfer the newly entered bedroom once, retaining the same run. */
BfmBedroom *bfm_chapter_completion_take_bedroom(BfmChapterCompletion *state,
                                                 const MusashiDiscMedia *disc);
#endif
