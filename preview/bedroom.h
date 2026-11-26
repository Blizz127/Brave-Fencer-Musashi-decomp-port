/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_H
#define BFM_BEDROOM_H
#include "bedroom_record.h"
#include "gameplay_run.h"

typedef struct BfmBedroom BfmBedroom;
BfmBedroom *bfm_bedroom_open(const MusashiDiscMedia *disc, const BfmSaveRecord *record);
/* Adopt the restored room flags into the process-owned bank after loading. */
void bfm_bedroom_bind_story_flags(BfmBedroom *room, uint8_t flags[64]);
void bfm_bedroom_bind_run(BfmBedroom *room, BfmGameplayRun *run);
void bfm_bedroom_free(BfmBedroom *room);
enum { BFM_BEDROOM_ADVANCED = 1, BFM_BEDROOM_VOICE_STOPPED };
int bfm_bedroom_step(BfmBedroom *room);
/* A stop result asks the desktop to cancel the queued voice output. */
/* Menu directions are digital. Field axes preserve stick deflection in -127..127. */
int bfm_bedroom_step_axes(BfmBedroom *room, int confirm_edge, int reveal_edge,
                          int direction, int held, int accept_edge, int back_edge, int x, int y, int talk_edge);
int bfm_bedroom_step_directions(BfmBedroom *room, int confirm_edge, int reveal_edge,
                                int direction, int held, int accept_edge, int back_edge, int horizontal);
int bfm_bedroom_step_pad(BfmBedroom *room, int confirm_edge, int reveal_edge,
                         int direction, int held, int accept_edge, int back_edge);
int bfm_bedroom_step_controls(BfmBedroom *room, int confirm_edge, int reveal_edge,
                              int direction, int accept_edge);
int bfm_bedroom_step_input(BfmBedroom *room, int confirm_edge, int reveal_edge);
/* Available while the keeper is offering service. */
const BfmSaveRecord *bfm_bedroom_inn_record(const BfmBedroom *room);
int bfm_bedroom_leave_inn(BfmBedroom *room);
int bfm_bedroom_draw_counter(BfmBedroom *room, int dialogue, uint32_t pixels[240][320]);
int bfm_bedroom_draw(BfmBedroom *room, uint32_t pixels[240][320]);
struct BfmBedroomEffects *bfm_bedroom_effect(const BfmBedroom *room, unsigned *command);
struct BfmForestScore *bfm_bedroom_score(const BfmBedroom *room);
struct BfmVillageImpactSound *bfm_bedroom_landing(const BfmBedroom *room);
struct BfmCableSound *bfm_bedroom_slider(const BfmBedroom *room, int *event);
int bfm_bedroom_endpoint(const BfmBedroom *room);
const char *bfm_bedroom_end_message(const BfmBedroom *room);
struct ForestVoice *bfm_bedroom_voice(BfmBedroom *room, unsigned *generation);
/* NTSC field of the latest mounted stream becoming ready on the room clock. */
unsigned bfm_bedroom_voice_ready_field(const BfmBedroom *room);
#endif
