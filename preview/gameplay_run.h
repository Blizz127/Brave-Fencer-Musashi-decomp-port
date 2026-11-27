/* SPDX-License-Identifier: MIT */
#ifndef BFM_GAMEPLAY_RUN_H
#define BFM_GAMEPLAY_RUN_H

#include "forest_status.h"
#include "bedroom_record.h"
#include <stdint.h>

typedef struct {
    uint8_t figure_prices[128];
    uint8_t variables[256];
    uint8_t timestamps[96];
} BfmGameplayBanks;

typedef enum {
    BFM_CHECKPOINT_SAVED_DATA, BFM_CHECKPOINT_MEMORY_BOX
} BfmCheckpointKind;

typedef struct {
    BfmSaveRecord state;
    int present;
} BfmFieldCheckpoint;

typedef struct BfmGameplayRun {
    /* The original startup clears this word once. Rooms share the stream. */
    uint32_t random_state;
    uint8_t story_flags[64];
    /* The ability, its HUD label and current BP travel with this playthrough. */
    BfmForestStatus player;
    int has_player;
    BfmGameplayBanks banks;
    BfmFieldCheckpoint saved_data, memory_box;
} BfmGameplayRun;

/* Initialize the first palace room from mounted New Game and figure data. */
int bfm_gameplay_new_player(BfmGameplayRun *run, const MusashiDiscMedia *disc);
/* The final Steam Knight cutscene advances the figure chapter before departure. */
int bfm_gameplay_advance_figures(BfmGameplayRun *run);

/* Saved data is refreshed after a successful save or load. The Memory Box
 * records its own destination on activation. Both belong to this run. */
void bfm_gameplay_remember_saved(BfmGameplayRun *run, const BfmSaveRecord *record);
int bfm_gameplay_remember_box(BfmGameplayRun *run, const BfmSaveRecord *record,
                              const uint8_t *box_destination, size_t bytes);
int bfm_gameplay_has_checkpoint(const BfmGameplayRun *run, BfmCheckpointKind kind);

/* Prepare the restored state before handing it to a receiving room. The
 * saved-data destination comes from the restored story on the mounted disc.
 * Failure leaves the current run and the caller's output untouched. */
int bfm_gameplay_woods_recovery(const BfmGameplayRun *run, BfmCheckpointKind kind,
                                const uint8_t *scene, size_t scene_bytes, BfmSaveRecord *out);

/* Count completed active scene updates in the saved player record. */
void bfm_gameplay_advance_playtime(uint8_t progress[152], unsigned updates);

uint16_t bfm_gameplay_random(BfmGameplayRun *run);
int bfm_gameplay_story_flag(const BfmGameplayRun *run, unsigned flag);

/* Commit the summit award once to the current playthrough. */
int bfm_gameplay_acquire_lumina(BfmGameplayRun *run);

#endif
