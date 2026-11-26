/* SPDX-License-Identifier: MIT */
#ifndef BFM_INN_GREETING_H
#define BFM_INN_GREETING_H
#include "bedroom_record.h"
#include "forest_message_art.h"

typedef struct BfmInnGreeting BfmInnGreeting;

enum {
    BFM_INN_HIDE_DIALOGUE, BFM_INN_NORMAL_DIALOGUE, BFM_INN_FIRST_ROOM_DESCRIPTION,
    BFM_INN_PAID_DIRECTIONS = BFM_INN_FIRST_ROOM_DESCRIPTION + 6,
    BFM_INN_LOW_FUNDS
};

/* Start at Hotelo's counter. The SC03 event, resident field member and
 * live player record remain borrowed until the room releases the conversation. */
BfmInnGreeting *bfm_inn_greeting_open(const uint8_t *module, size_t bytes,
                                     const uint8_t *resident, size_t resident_bytes, BfmSaveRecord *record);
void bfm_inn_greeting_free(BfmInnGreeting *talk);
int bfm_inn_greeting_step(BfmInnGreeting *talk, uint16_t pressed, unsigned voice_status);
const BfmForestMessageFrame *bfm_inn_greeting_frame(const BfmInnGreeting *talk);
/* The most recent tick, including voice events from a window that closed. */
const BfmForestMessageFrame *bfm_inn_greeting_update(const BfmInnGreeting *talk);
int bfm_inn_greeting_service(const BfmInnGreeting *talk);
/* The selected response reached the end of this preview at the counter. */
int bfm_inn_greeting_leaving(const BfmInnGreeting *talk);
int bfm_inn_greeting_draw(const BfmInnGreeting *talk, int dialogue, const BfmForestMessageArt *art,
                          const uint16_t tiles[512][1024], uint32_t pixels[240][320]);
#endif
