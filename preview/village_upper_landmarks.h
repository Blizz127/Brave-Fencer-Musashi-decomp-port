/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_UPPER_LANDMARKS_H
#define BFM_VILLAGE_UPPER_LANDMARKS_H
#include "bedroom_field.h"
#include "forest_message.h"

/* The three timber signs share their shape and material. Each placement
 * supplies its own heading and reading. Keep the scene mounted until free. */
typedef struct BfmVillageUpperLandmarks BfmVillageUpperLandmarks;
BfmVillageUpperLandmarks *bfm_village_upper_landmarks_open(const uint8_t *scene,
                                                           size_t bytes, uint32_t room);
void bfm_village_upper_landmarks_free(BfmVillageUpperLandmarks *landmarks);
int bfm_village_upper_landmarks_prepare(BfmVillageUpperLandmarks *landmarks,
        const BfmBedroomView *view, const uint16_t tiles[512][1024], uint32_t *pixels);
int bfm_village_upper_landmarks_next_depth(const BfmVillageUpperLandmarks *landmarks);
void bfm_village_upper_landmarks_draw_to(BfmVillageUpperLandmarks *landmarks, int depth);
int bfm_village_upper_landmarks_slide(const BfmVillageUpperLandmarks *landmarks,
                                      const int16_t previous[3], int16_t endpoint[3]);
/* Call on an ordinary, fresh interaction after the room releases the player.
 * The room's window takes focus/close in the usual way. NULL means no reading. */
BfmForestMessage *bfm_village_upper_landmarks_read(const BfmVillageUpperLandmarks *landmarks,
                                                    const int16_t hero[3], int square_pressed,
                                                    int16_t point[3]);
#endif
