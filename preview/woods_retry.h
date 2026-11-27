/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_RETRY_H
#define BFM_WOODS_RETRY_H

#include "gameplay_run.h"
#include "forest_message.h"

typedef enum {
    BFM_WOODS_RETRY_CHOOSE, BFM_WOODS_RETRY_BOX, BFM_WOODS_RETRY_SAVED,
    BFM_WOODS_RETRY_MISSING, BFM_WOODS_RESTORE_BOX, BFM_WOODS_RESTORE_SAVED,
    BFM_WOODS_RESTART_CHAPTER, BFM_WOODS_RETRY_INVALID
} BfmWoodsRetryPage;

/* Enter after Musashi's defeat line closes. Report the original chapter-reset
 * branch separately when neither working checkpoint has been created. */
BfmWoodsRetryPage bfm_woods_retry_begin(const BfmGameplayRun *run);

/* Only a closed message supplies an answer. Its first selectable row is 1. */
BfmWoodsRetryPage bfm_woods_retry_answer(const BfmGameplayRun *run,
                                        BfmWoodsRetryPage page, unsigned answer);

/* Load each displayed window from the player's disc. Terminal pages do not
 * have a window; the field must prepare the requested room separately. */
BfmForestMessage *bfm_woods_retry_window(const MusashiDiscMedia *disc, BfmWoodsRetryPage page);

/* Verify the restored destination and its mounted member together. The field
 * takes ownership only after the receiving scene has opened successfully. */
int bfm_woods_retry_field_destination(const MusashiDiscMedia *disc, const BfmGameplayRun *run,
                                     BfmWoodsRetryPage page, const uint8_t *scene, size_t bytes,
                                     size_t saved_destination_offset, BfmBedroomEntry *out);
int bfm_woods_retry_destination(const MusashiDiscMedia *disc, const BfmGameplayRun *run,
                                BfmWoodsRetryPage page, const uint8_t *woods_scene,
                                size_t scene_bytes, BfmBedroomEntry *out);

#endif
