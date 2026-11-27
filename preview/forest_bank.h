/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BANK_H
#define BFM_FOREST_BANK_H

#include "bedroom_record.h"
#include "village_arrival_motion.h"
#include "village_camera.h"
#include "field_input.h"
#include "gameplay_run.h"
#include "retry_screen.h"

typedef struct BfmForestBank BfmForestBank;

/* The caller owns the expanded SC03 member 6 and texture memory throughout the visit. */
BfmForestBank *bfm_forest_bank_open(const MusashiDiscMedia *disc, const uint8_t *scene,
                                   size_t bytes, uint16_t tiles[512][1024], unsigned phase);
void bfm_forest_bank_free(BfmForestBank *bank);
void bfm_forest_bank_bind_player(BfmForestBank *bank, const uint8_t *main, size_t bytes);
void bfm_forest_bank_step_light(BfmForestBank *bank, unsigned phase);
/* When the hurt pose ends in a fatal hit, stop this field update. The next
 * update checks the working inventory before offering defeat. */
enum { BFM_BANK_HIT_FATAL = 2 };
int bfm_forest_bank_receive(BfmForestBank *bank, BfmVillageMotion *motion,
                            BfmGameplayRun *run, BfmFieldInput input, unsigned actor_tick);
enum { BFM_BANK_RECOVERY_ERROR = -1, BFM_BANK_PLAYING, BFM_BANK_WAITING, BFM_BANK_REVIVED };
/* A revived player resumes movement on this field with the old pad cleared. */
int bfm_forest_bank_recovery_step(BfmForestBank *bank, BfmVillageMotion *motion,
                                  BfmGameplayRun *run, BfmSaveRecord *save,
                                  BfmForestMessage **opened);
int bfm_forest_bank_begin_retry(BfmForestBank *bank, const MusashiDiscMedia *disc,
                                const uint8_t *scene, size_t scene_bytes,
                                const BfmVillageMotion *motion, const BfmSaveRecord *save,
                                const BfmVillageCamera *camera, const unsigned *age,
                                BfmGameplayRun *run);
BfmRetryScreen *bfm_forest_bank_retry(const BfmForestBank *bank);
int bfm_forest_bank_draw_retry(BfmForestBank *bank, const BfmVillageMotion *motion,
                               const BfmVillageCamera *camera, const BfmForestHud *hud,
                               uint32_t pixels[240][320]);
void bfm_forest_bank_draw_message(const BfmForestBank *bank, const BfmForestMessageFrame *window,
                                  uint32_t pixels[240][320]);
int bfm_forest_bank_awarding(const BfmForestBank *bank);
int bfm_forest_bank_movement_locked(const BfmForestBank *bank);
unsigned bfm_forest_bank_sound(const BfmForestBank *bank);
unsigned bfm_forest_bank_take_sound(BfmForestBank *bank);
void bfm_forest_bank_step_actors(BfmForestBank *bank, const BfmBedroomRoot *player,
                                 BfmGameplayRun *run, BfmSaveRecord *save);
int bfm_forest_bank_cast(BfmForestBank *bank, BfmVillageMotion *motion, BfmGameplayRun *run,
                         const BfmVillageCamera *camera, BfmSaveRecord *save, BfmFieldInput input,
                         unsigned field_tick, unsigned actor_tick, const int16_t previous[3],
                         BfmForestMessage **opened);
void bfm_forest_bank_hop_sound(BfmForestBank *bank);
int bfm_forest_bank_move(BfmForestBank *bank, BfmVillageMotion *motion,
                         BfmGameplayRun *run, BfmFieldInput field, int camera_yaw);
int16_t bfm_forest_bank_tracking_y(const BfmForestBank *bank, int16_t foot_y);
int bfm_forest_bank_body_available(const BfmForestBank *bank);
int bfm_forest_bank_close(BfmForestBank *bank);
int bfm_forest_bank_tether(const BfmForestBank *bank);
int bfm_forest_bank_award_cue(const BfmForestBank *bank);
int bfm_forest_bank_draw(BfmForestBank *bank, const BfmVillageMotion *motion,
                         const BfmVillageCamera *camera, const BfmSaveRecord *save,
                         unsigned age, uint32_t pixels[240][320]);
#endif
