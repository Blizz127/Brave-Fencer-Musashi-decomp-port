/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_DEFEAT_H
#define BFM_VALLEY_DEFEAT_H

#include "village_field_defeat.h"

typedef struct BfmValleyDefeat BfmValleyDefeat;

typedef enum {
    BFM_VALLEY_DEFEAT_ERROR, BFM_VALLEY_REVIVED, BFM_VALLEY_RETRY
} BfmValleyDefeatResult;

/* The Valley retains its mounted room, tiles, gameplay clock and field until
 * the recovery is freed. Retry additionally borrows the outgoing motion/save. */
BfmValleyDefeat *bfm_valley_defeat_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t scene_bytes, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024],
    BfmVillageField *field, const unsigned *age);
void bfm_valley_defeat_free(BfmValleyDefeat *defeat);

/* Call on the first active player update following a lethal pit return.
 * On REVIVED the caller clears the previous pad and owns the opened notice.
 * On RETRY the room keeps its outgoing camera and the desktop takes over. */
BfmForestMessageArt *bfm_valley_defeat_window_art(BfmValleyDefeat *defeat);
BfmValleyDefeatResult bfm_valley_defeat_resolve(BfmValleyDefeat *defeat, BfmVillageMotion *motion,
    BfmSaveRecord *save, BfmGameplayRun *run, const BfmVillageCamera *presented,
    BfmForestMessage **opened);
BfmRetryScreen *bfm_valley_defeat_retry(const BfmValleyDefeat *defeat);
void bfm_valley_defeat_draw_notice(const BfmValleyDefeat *defeat, const BfmForestMessageFrame *notice,
                                  uint32_t pixels[240][320]);
/* The caller draws the outgoing world with its normal body hidden while
 * bfm_retry_screen_keep_world is true. The shared screen draws the collapse. */
int bfm_valley_defeat_draw_retry(const BfmValleyDefeat *defeat, const BfmForestHud *hud,
                               uint32_t pixels[240][320]);
int bfm_valley_defeat_destination(const BfmValleyDefeat *defeat, const BfmGameplayRun *run,
                                BfmBedroomEntry *out);

#endif
