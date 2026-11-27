/* SPDX-License-Identifier: MIT */
#ifndef BFM_BAKERY_FIELD_H
#define BFM_BAKERY_FIELD_H

#include "forest_hud.h"
#include "forest_message.h"
#include "gameplay_run.h"

typedef struct BfmBakeryField BfmBakeryField;
BfmBakeryField *bfm_bakery_field_open(const MusashiDiscMedia *disc, const uint8_t *door,
                                     BfmSaveRecord *record, const uint8_t *main, size_t main_bytes,
                                     uint16_t tiles[512][1024], uint16_t *service_clock);
void bfm_bakery_field_free(BfmBakeryField *shop);
int bfm_bakery_field_step(BfmBakeryField *shop, uint16_t pressed, uint16_t held_directions,
                          unsigned voice_status, BfmGameplayRun *run);
int bfm_bakery_field_draw(BfmBakeryField *shop, uint32_t pixels[240][320]);
void bfm_bakery_field_draw_hud(const BfmBakeryField *shop, BfmForestHud *hud,
                               const BfmForestStatus *status, uint32_t pixels[240][320]);
void bfm_bakery_field_shade(const BfmBakeryField *shop, uint32_t pixels[240][320]);
const BfmForestMessageFrame *bfm_bakery_field_window(const BfmBakeryField *shop);
const uint8_t *bfm_bakery_field_departure(const BfmBakeryField *shop);
unsigned bfm_bakery_field_sound(const BfmBakeryField *shop);
#endif
