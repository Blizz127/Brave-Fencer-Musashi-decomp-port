/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_UPPER_GUARD_H
#define BFM_VILLAGE_UPPER_GUARD_H

#include "bedroom_figure.h"
#include "bedroom_record.h"
#include "forest_message.h"

typedef struct BfmVillageUpperGuard BfmVillageUpperGuard;

/* Lumpwood registers at the upper Twinpeak passage after his rescue.
 * Call open only when present. Keep the mounted SC03/2 scene alive. */
int bfm_village_upper_guard_present(const BfmSaveRecord *state);
BfmVillageUpperGuard *bfm_village_upper_guard_open(const uint8_t *scene, size_t bytes,
                                                   const BfmSaveRecord *state);
void bfm_village_upper_guard_free(BfmVillageUpperGuard *guard);
int bfm_village_upper_guard_add(const BfmVillageUpperGuard *guard,
                                BfmBedroomFigure *figure, const BfmBedroomView *view);
/* Load after the room's MAIN and SC03 texture pages. The copy of the
 * 32x40 resident face belongs to this actor's message art. */
int bfm_village_upper_guard_load_art(BfmVillageUpperGuard *guard, const MusashiDiscMedia *disc,
                                     uint16_t tiles[512][1024]);
int bfm_village_upper_guard_draw_message(const BfmVillageUpperGuard *guard,
    const BfmBedroomView *view, const int16_t listener[3], const BfmForestMessageFrame *frame,
    uint32_t pixels[240][320]);
int bfm_village_upper_guard_contact(const BfmVillageUpperGuard *guard,
                                    const int16_t previous[3], int16_t endpoint[3]);

typedef struct {
    int first;
    int16_t speaker[3], approach[3];
    unsigned speaker_yaw;
    const uint8_t *camera; /* First conversation's 16-byte orbit data. */
} BfmVillageGuardTalk;

/* Supply the ordinary focused Square edge only with player field control.
 * A first meeting requests the actor-relative camera and approach before the
 * line opens. Later meetings during this visit use the same line in place. */
int bfm_village_upper_guard_begin(BfmVillageUpperGuard *guard, BfmSaveRecord *save,
                                  const int16_t player[3], int square, BfmVillageGuardTalk *talk);
/* The caller owns the returned window. On the first meeting wait for the
 * actual player's staged approach to finish. Consume its real close event. */
BfmForestMessage *bfm_village_upper_guard_message(BfmVillageUpperGuard *guard, int approach_ready);
void bfm_village_upper_guard_close(BfmVillageUpperGuard *guard);

/* Once per native actor update, including during dialogue and the arrival
 * fade. A return value of 1 requests release of the conversation camera.
 * Notify camera_done when that return finishes. */
int bfm_village_upper_guard_step(BfmVillageUpperGuard *guard);
void bfm_village_upper_guard_camera_done(BfmVillageUpperGuard *guard);
int bfm_village_upper_guard_busy(const BfmVillageUpperGuard *guard);

#endif
