/* SPDX-License-Identifier: MIT */
#ifndef BFM_BAKERY_SPEECH_H
#define BFM_BAKERY_SPEECH_H

#include "bedroom_record.h"
#include "forest_message_art.h"

typedef struct BfmBakerySpeech BfmBakerySpeech;

/* Borrow the mounted SC03/74 event payload and the player's live save record.
 * Begin at the counter after the entry walk reaches its destination. */
BfmBakerySpeech *bfm_bakery_speech_open(const uint8_t *module, size_t bytes, BfmSaveRecord *record);
void bfm_bakery_speech_free(BfmBakerySpeech *speech);
int bfm_bakery_speech_step(BfmBakerySpeech *speech, uint16_t pressed, unsigned voice_status);
const BfmForestMessageFrame *bfm_bakery_speech_frame(const BfmBakerySpeech *speech);
/* Inspect just-issued voice commands even when a window closed on this field. */
const BfmForestMessageFrame *bfm_bakery_speech_update(const BfmBakerySpeech *speech);
int bfm_bakery_speech_hero(const BfmBakerySpeech *speech);
int bfm_bakery_speech_service(const BfmBakerySpeech *speech);
int bfm_bakery_speech_finish_service(BfmBakerySpeech *speech);
int bfm_bakery_speech_leaving(const BfmBakerySpeech *speech);

typedef enum {
    BFM_BAKERY_CAPTION_OFFER, BFM_BAKERY_CAPTION_ITEM, BFM_BAKERY_CAPTION_BOUGHT, BFM_BAKERY_CAPTION_FULL, BFM_BAKERY_CAPTION_POOR,
    BFM_BAKERY_CAPTION_SOLD, BFM_BAKERY_CAPTION_REFUSED, BFM_BAKERY_CAPTION_EMPTY,
    BFM_BAKERY_CAPTION_SALE_OFFER
} BfmBakeryCaption;
/* Advance once after processing the service input and before drawing. */
void bfm_bakery_speech_service_tick(BfmBakerySpeech *speech, BfmBakeryCaption caption,
                                   unsigned item, unsigned sale_slot, int sale_tens);
/* The service message uses the live item's number, the lower-village data,
 * and the MAIN/10 field payload. The Sell panel supplies its inventory slot
 * and current signed quote in tens of Drans. */
int bfm_bakery_speech_draw(const BfmBakerySpeech *speech, BfmBakeryCaption caption, unsigned item,
                           unsigned sale_slot, int sale_tens,
                           const BfmForestMessageArt *art, const uint8_t *resident, size_t resident_bytes,
                           const uint8_t *main, size_t main_bytes, const uint16_t tiles[512][1024],
                           uint32_t pixels[240][320]);

#endif
