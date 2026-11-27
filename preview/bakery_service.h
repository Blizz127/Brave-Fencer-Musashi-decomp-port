/* SPDX-License-Identifier: MIT */
#ifndef BFM_BAKERY_SERVICE_H
#define BFM_BAKERY_SERVICE_H

#include "bakery_menu_art.h"
#include "bakery_purchase.h"
#include "bakery_speech.h"

typedef struct {
    BfmBakeryPurchase stock;
    BfmBakeryPane pane;
    BfmBakeryCaption caption;
    unsigned service, good, first_good, item, quote_drans;
    /* One room sound and its argument per service field. Zero means silence. */
    unsigned sound, sound_arg;
    const uint8_t *main;
    size_t main_bytes;
    uint16_t last_held;
    unsigned repeat_wait;
} BfmBakeryService;

typedef enum {
    BFM_BAKERY_SERVICE_ERROR, BFM_BAKERY_SERVICE_ACTIVE, BFM_BAKERY_SERVICE_QUIT
} BfmBakeryServiceResult;

/* Begin each visit to Buy/Sell/Quit after the counter dialogue. The service
 * borrows mounted MAIN player data for inventory names and sale prices. */
int bfm_bakery_service_begin(BfmBakeryService *service, const uint8_t *scene, size_t scene_bytes,
                             const uint8_t *main, size_t main_bytes, const BfmSaveRecord *record);
/* The pad reader provides fresh face edges. Directions can be supplied as
 * axes held across fields. Their first edge and ordinary repeats are emitted. */
uint16_t bfm_bakery_service_pad(BfmBakeryService *service, uint16_t pressed, uint16_t held);
BfmBakeryServiceResult bfm_bakery_service_step(BfmBakeryService *service, uint16_t pressed,
                                              BfmSaveRecord *record, BfmGameplayRun *run);
int bfm_bakery_service_view(const BfmBakeryService *service, const BfmSaveRecord *record,
                            BfmBakeryMenuView *view);

/* Item number for the selected description. Zero selects no item. */
unsigned bfm_bakery_service_item(const BfmBakeryService *service, const BfmSaveRecord *record);
#endif
