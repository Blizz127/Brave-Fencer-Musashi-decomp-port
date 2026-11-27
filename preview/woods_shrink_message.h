/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SHRINK_MESSAGE_H
#define BFM_WOODS_SHRINK_MESSAGE_H

#include "forest_message.h"

enum { BFM_WOODS_MESSAGE_SATIATE = 3, BFM_WOODS_MESSAGE_HOP = 12, BFM_WOODS_MESSAGE_SHRINK = 13 };

/* Call when the returning blade delivers the absorbed ability.
 * Supply SC03 member 6 for Hop, or member 29 for Satiate and Shrink.
 * The mounted MAIN member 10 player payload must outlive the cursor. The field message player
 * advances the pages on confirm edges. Equip on BFM_FOREST_MESSAGE_CLOSED. */
BfmForestMessage *bfm_woods_ability_message_open(const uint8_t *scene, size_t scene_bytes,
                                                const uint8_t *main, size_t main_bytes,
                                                unsigned ability);

BfmForestMessage *bfm_woods_shrink_message_open(const uint8_t *scene, size_t scene_bytes,
                                               const uint8_t *main, size_t main_bytes);

#endif
