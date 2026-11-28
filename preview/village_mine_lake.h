/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_H
#define BFM_VILLAGE_MINE_LAKE_H

#include "village_arrival_motion.h"
#include "field_input.h"
#include "village_mine_view.h"

typedef struct BfmVillageMineLake BfmVillageMineLake;
typedef struct BfmMineLakeHurt BfmMineLakeHurt;
typedef void (*BfmMineLakeSound)(void *context, unsigned command, unsigned value);

BfmVillageMineLake *bfm_village_mine_lake_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    uint16_t tiles[512][1024], BfmVillageMotion *motion, BfmSaveRecord *save);
void bfm_village_mine_lake_free(BfmVillageMineLake *lake);
BfmVillageMineView *bfm_village_mine_lake_view(const BfmVillageMineLake *lake);
BfmMineLakeHurt *bfm_village_mine_lake_hurt(const BfmVillageMineLake *lake);
int bfm_village_mine_lake_player(BfmVillageMineLake *lake, BfmGameplayRun *run, BfmFieldInput input,
    int pad_yaw, unsigned tick, BfmMineLakeSound emit, void *context);
int bfm_village_mine_lake_actors(BfmVillageMineLake *lake, BfmGameplayRun *run, int camera_pressed,
    unsigned tick, BfmMineLakeSound emit, void *context, int collapsing);
int bfm_village_mine_lake_draw(BfmVillageMineLake *lake, unsigned tick, unsigned scale, int hidden,
    uint32_t pixels[240][320]);

#endif
