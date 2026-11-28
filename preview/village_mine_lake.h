/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_H
#define BFM_VILLAGE_MINE_LAKE_H

#include "village_arrival_motion.h"
#include "field_input.h"
#include "village_mine_view.h"
#include "village_mine_lake_fields.h"

typedef struct BfmVillageMineLake BfmVillageMineLake;
typedef struct BfmMineLakeHurt BfmMineLakeHurt;
typedef void (*BfmMineLakeSound)(void *context, unsigned command, unsigned value);

BfmVillageMineLake *bfm_village_mine_lake_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    uint16_t tiles[512][1024], BfmVillageMotion *motion, BfmSaveRecord *save);
void bfm_village_mine_lake_free(BfmVillageMineLake *lake);
BfmVillageMineView *bfm_village_mine_lake_view(const BfmVillageMineLake *lake);
BfmMineLakeHurt *bfm_village_mine_lake_hurt(const BfmVillageMineLake *lake);
/* New input and the village clock stay locked until the actor's own rescue
 * script registers its saved variable. Read both events after room actors. */
int bfm_village_mine_lake_rescue_busy(const BfmVillageMineLake *lake);
const BfmVillageMineLakeFieldEvent *bfm_village_mine_lake_rescue_events(const BfmVillageMineLake *lake);
/* Optional callbacks borrow the controller through this visit. Camera runs
 * after ordinary follow, and resident geometry joins the room actor queue. */
void bfm_village_mine_lake_rescue_hooks(BfmVillageMineLake *lake, void *context,
    void (*camera)(void *, BfmVillageCamera *),
    int (*draw)(void *, BfmBedroomFigure *, const BfmBedroomView *));
int bfm_village_mine_lake_rescue_player_ready(const BfmVillageMineLake *lake);
/* Call when the approach script takes over the standing actor's rendering. */
void bfm_village_mine_lake_rescue_claim_body(BfmVillageMineLake *lake, unsigned resident_id);
int bfm_village_mine_lake_player(BfmVillageMineLake *lake, BfmGameplayRun *run, BfmFieldInput input,
    int pad_yaw, unsigned tick, BfmMineLakeSound emit, void *context);
int bfm_village_mine_lake_player_scripted(BfmVillageMineLake *lake, BfmGameplayRun *run, BfmFieldInput input,
    int pad_yaw, unsigned tick, BfmMineLakeSound emit, void *context, int drives_player);
int bfm_village_mine_lake_player_claimed(const BfmVillageMineLake *lake);
int bfm_village_mine_lake_actors(BfmVillageMineLake *lake, BfmGameplayRun *run, int camera_pressed,
    unsigned tick, BfmMineLakeSound emit, void *context, int collapsing);
int bfm_village_mine_lake_draw(BfmVillageMineLake *lake, unsigned tick, unsigned scale, int hidden,
    uint32_t pixels[240][320]);

#endif
