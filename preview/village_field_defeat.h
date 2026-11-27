/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_FIELD_DEFEAT_H
#define BFM_VILLAGE_FIELD_DEFEAT_H

#include "retry_screen.h"
#include "village_arrival_motion.h"
#include "village_field.h"

/* Embed this context in the active room. Its fields, including the current
 * scene age and texture pages, are borrowed until the retry screen is freed. */
typedef struct {
    BfmVillageField *field;
    const BfmVillageMotion *motion;
    const BfmSaveRecord *save;
    const uint8_t *main;
    size_t main_bytes;
    uint16_t (*tiles)[1024];
    const unsigned *age;
} BfmVillageFieldDefeat;

/* Use this field's presented camera and its mounted dialogue and light table.
 * During the outgoing wipe the room draws the world and ground shadow first,
 * with the ordinary body hidden, then bfm_retry_screen_draw adds this actor. */
BfmRetryScreen *bfm_village_field_defeat_open(BfmVillageFieldDefeat *actor,
    const MusashiDiscMedia *disc, const uint8_t *scene, size_t scene_bytes, size_t table_offset,
    const BfmVillageCamera *presented, BfmGameplayRun *run);

#endif
