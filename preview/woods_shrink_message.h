/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SHRINK_MESSAGE_H
#define BFM_WOODS_SHRINK_MESSAGE_H

#include "forest_message.h"

/* Call when the returning blade delivers Shrink in southern Cleft Woods.
 * scene is SC03 member 29's field payload. main is the mounted MAIN member
 * 10 player payload and must outlive the cursor. The field message player
 * advances the pages on confirm edges. Equip on BFM_FOREST_MESSAGE_CLOSED. */
BfmForestMessage *bfm_woods_shrink_message_open(const uint8_t *scene, size_t scene_bytes,
                                               const uint8_t *main, size_t main_bytes);

#endif
