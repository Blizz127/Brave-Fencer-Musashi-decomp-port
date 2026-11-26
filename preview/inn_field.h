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
int bfm_inn_field_service(const BfmInnField *inn);
int bfm_inn_field_draw(BfmInnField *inn, int show_dialogue, uint32_t pixels[240][320]);
const BfmForestMessageFrame *bfm_inn_field_window(const BfmInnField *inn);
void bfm_inn_field_voice_status(BfmInnField *inn, unsigned status);
unsigned bfm_inn_room_price(const BfmInnField *inn, unsigned room);
int bfm_inn_pay_room201(BfmInnField *inn);
void bfm_inn_finish_room201(BfmInnField *inn);
void bfm_inn_field_leave(BfmInnField *inn);
/* Current proven endpoint; keep drawing the counter until the menu returns to title. */
int bfm_inn_field_finished(const BfmInnField *inn);
#endif
