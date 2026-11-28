/* SPDX-License-Identifier: MIT */
#ifndef BFM_NIGHTMARE_DEPARTURE_H
#define BFM_NIGHTMARE_DEPARTURE_H

#include "bedroom_field.h"
#include "bedroom_motion.h"
#include "bedroom_travel.h"
#include "nightmare_room.h"
#include "nightmare_field.h"

typedef struct BfmNightmareDeparture BfmNightmareDeparture;

/* All mounted data and player/field objects remain with the room. Call step
 * first on the close-shot update, after the wake effect and wide-view waits
 * have completed (active player update 171). */
BfmNightmareDeparture *bfm_nightmare_departure_open(const uint8_t *scene, size_t bytes,
                                 uint32_t room_address, const BfmNightmareLayout *layout, BfmBedroomMotion *motion,
                                 BfmBedroomField *field, BfmBedroomRoot *root, BfmBedroomPose *pose);
void bfm_nightmare_departure_free(BfmNightmareDeparture *d);
/* Advances the room's OUT voice service and applies each window's requests.
 * other_transfer is the original room's competing-transfer latch. */
int bfm_nightmare_departure_step(BfmNightmareDeparture *d, ForestVoice *voice,
                                 unsigned *generation, int confirm, int reveal, int other_transfer,
                                 BfmNightmareRoomEvent *event);
const BfmForestMessageFrame *bfm_nightmare_departure_message(const BfmNightmareDeparture *d);
#endif
