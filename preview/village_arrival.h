/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ARRIVAL_H
#define BFM_VILLAGE_ARRIVAL_H
#include "forest_hud.h"
#include "field_input.h"
#include "bedroom_record.h"
#include "gameplay_run.h"
#include "retry_screen.h"
#include "forest_message.h"
#include "village_arrival_motion.h"

typedef struct BfmVillageArrival BfmVillageArrival;
BfmVillageArrival *bfm_village_arrival_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                      const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024]);
BfmVillageArrival *bfm_village_resume_inn(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                         const uint8_t *main, size_t bytes,
                                         uint16_t tiles[512][1024], BfmForestHud **hud);
void bfm_village_arrival_free(BfmVillageArrival *arrival);
uint16_t bfm_village_arrival_room(const BfmVillageArrival *arrival);
struct BfmSouthernWoods *bfm_village_arrival_woods(BfmVillageArrival *arrival);
BfmRetryScreen *bfm_village_arrival_retry(const BfmVillageArrival *arrival);
int bfm_village_arrival_begin_retry(BfmVillageArrival *arrival, BfmGameplayRun *run);
int bfm_village_arrival_draw_retry(BfmVillageArrival *arrival, const BfmForestHud *hud,
                                 uint32_t pixels[240][320]);
int bfm_village_arrival_prepare_recovery(const BfmVillageArrival *arrival, const BfmGameplayRun *run,
                                        BfmBedroomEntry *out);
/* Restore a validated, earned working Memory Box checkpoint in the same
 * playthrough. The outgoing field and its Retry screen survive a failed mount. */
int bfm_village_arrival_restore_woods_box(BfmVillageArrival *arrival, const BfmBedroomEntry *restored);
const BfmVillagePlayer *bfm_village_arrival_player(const BfmVillageArrival *arrival);
int bfm_village_arrival_step(BfmVillageArrival *arrival, BfmGameplayRun *run, int x, int y);
/* An ordinary shop exit inherits the orbit left on the street before entering. */
void bfm_village_arrival_return_from_shop(BfmVillageArrival *outside,
                                         const BfmVillageArrival *street);
/* Outdoor actors update during the inn exit fade, before player control. */
void bfm_village_arrival_lower_register(BfmVillageArrival *a, BfmGameplayRun *run);
void bfm_village_arrival_lower_ambient(BfmVillageArrival *a, BfmGameplayRun *run);
int bfm_village_arrival_step_field(BfmVillageArrival *arrival, BfmGameplayRun *run,
                                   BfmFieldInput field, BfmForestMessage **opened);
int bfm_village_arrival_award_cue(const BfmVillageArrival *arrival);
void bfm_village_arrival_woods_score_applied(BfmVillageArrival *arrival, unsigned selector);
unsigned bfm_village_arrival_woods_score_selector(const BfmVillageArrival *a);
/* Positive selects a track, zero releases the field override, negative requests silence. */
int bfm_village_arrival_field_score(const BfmVillageArrival *a, unsigned village_score);
unsigned bfm_village_arrival_take_score_command(BfmVillageArrival *a);
unsigned bfm_village_arrival_take_upper_sound(BfmVillageArrival *a);
int bfm_village_arrival_take_retired_window(BfmVillageArrival *arrival);
unsigned bfm_village_arrival_take_box_voice(BfmVillageArrival *arrival);
unsigned bfm_village_arrival_take_box_sound(BfmVillageArrival *arrival);
unsigned bfm_village_arrival_woods_sound(const BfmVillageArrival *arrival);
enum { BFM_VILLAGE_WOODS_SOUNDS = 16 };
size_t bfm_village_arrival_take_woods_sounds(BfmVillageArrival *arrival, unsigned *out,
                                            size_t capacity);
int bfm_village_arrival_tether_gauge(const BfmVillageArrival *arrival);
int bfm_village_arrival_take_hud(BfmVillageArrival *arrival);
int bfm_village_arrival_rest_hud(const BfmVillageArrival *arrival, BfmForestHud *hud);
int bfm_village_arrival_detector(const BfmVillageArrival *arrival);
int bfm_village_arrival_step_input(BfmVillageArrival *arrival, BfmGameplayRun *run,
                                   int x, int y, int talk_edge, BfmForestMessage **opened);
void bfm_village_arrival_begin_hotelo_reveal(BfmVillageArrival *arrival);
void bfm_village_arrival_set_scene_tick(BfmVillageArrival *arrival, unsigned tick);
const uint8_t *bfm_village_arrival_castle_transfer(const BfmVillageArrival *arrival);
const uint8_t *bfm_village_arrival_shop_transfer(const BfmVillageArrival *arrival);
const uint8_t *bfm_village_arrival_inn_transfer(const BfmVillageArrival *arrival);
int bfm_village_arrival_forest_score_fades(const BfmVillageArrival *arrival);
int bfm_village_arrival_maze_crossing(const BfmVillageArrival *arrival);
unsigned bfm_village_arrival_reveal_tick(const BfmVillageArrival *arrival);
unsigned bfm_village_arrival_transfer_tick(const BfmVillageArrival *arrival);
/* Submit after the field and HUD have rendered; composite onto the
 * presented page before other room wipes. Elapsed playtime resumes after
 * the receiving field's last shade update. */
int bfm_village_arrival_woods_playtime_paused(const BfmVillageArrival *arrival);
void bfm_village_arrival_submit_woods_crossing(BfmVillageArrival *arrival, const uint32_t submitted[240][320],
                                              const uint32_t presented[240][320]);
int bfm_village_arrival_draw_woods_crossing(const BfmVillageArrival *arrival, uint32_t pixels[240][320]);
int bfm_village_arrival_message_closed(BfmVillageArrival *arrival, unsigned answer);
int bfm_village_arrival_continue(BfmVillageArrival *arrival);
BfmForestMessage *bfm_village_arrival_speech(const BfmVillageArrival *arrival);
int bfm_village_arrival_draw(BfmVillageArrival *arrival, const uint8_t flags[64],
                             const BfmForestMessageFrame *window, uint32_t pixels[240][320]);
int bfm_village_arrival_draw_page(BfmVillageArrival *arrival, const uint8_t flags[64],
                                  const BfmForestMessageFrame *window, unsigned draw_page,
                                  uint32_t pixels[240][320]);
#endif
