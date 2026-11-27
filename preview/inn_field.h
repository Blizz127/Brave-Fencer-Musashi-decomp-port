/* SPDX-License-Identifier: MIT */
#ifndef BFM_INN_FIELD_H
#define BFM_INN_FIELD_H
#include "bedroom_record.h"
#include "forest_message.h"

typedef struct BfmInnField BfmInnField;

/* Enter the mounted Hotelo room from its lower-village doorway. The player
 * owns the save record and the shared texture allocation. */
BfmInnField *bfm_inn_field_open(const MusashiDiscMedia *disc, const uint8_t *transfer,
                                BfmSaveRecord *record, uint16_t tiles[512][1024]);
void bfm_inn_field_free(BfmInnField *inn);
int bfm_inn_field_step(BfmInnField *inn, uint16_t pressed);
struct BfmGameplayRun;
int bfm_inn_field_step_run(BfmInnField *inn, uint16_t pressed, struct BfmGameplayRun *run);
int bfm_inn_field_service(const BfmInnField *inn);
int bfm_inn_field_draw(BfmInnField *inn, int show_dialogue, uint32_t pixels[240][320]);
const BfmForestMessageFrame *bfm_inn_field_window(const BfmInnField *inn);
void bfm_inn_field_voice_status(BfmInnField *inn, unsigned status);
/* Borrowed while Hotelo and its shared texture allocation remain open. */
const uint16_t *bfm_inn_field_menu_palette(const BfmInnField *inn);
unsigned bfm_inn_room_price(const BfmInnField *inn, unsigned room);
int bfm_inn_pay_room201(BfmInnField *inn);
void bfm_inn_finish_room201(BfmInnField *inn);
int bfm_inn_field_leave(BfmInnField *inn);
/* Apply the doorway fade after the indoor scene and HUD have been drawn. */
void bfm_inn_field_shade(const BfmInnField *inn, uint32_t pixels[240][320]);
/* Borrow the mounted lower-village transfer once the indoor walk and fade finish. */
const uint8_t *bfm_inn_field_departure(const BfmInnField *inn);
/* Later story conversations outside the mounted Hotelo route end at the counter. */
int bfm_inn_field_finished(const BfmInnField *inn);
#endif
