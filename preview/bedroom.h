/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_H
#define BFM_BEDROOM_H
#include "bedroom_record.h"
#include "gameplay_run.h"
#include "field_input.h"
#include "retry_screen.h"
#include "woods_retry.h"

typedef struct BfmBedroom BfmBedroom;
BfmBedroom *bfm_bedroom_open(const MusashiDiscMedia *disc, const BfmSaveRecord *record);
/* The defeat branch supplies a MAIN player transfer for the same castle
 * wake. Preserve its room callbacks independently of a save's Continue row. */
BfmBedroom *bfm_bedroom_open_restart(const MusashiDiscMedia *disc, const BfmSaveRecord *record);
/* Enter the new chapter on the existing run, including the wake script's
 * resource changes. The retained chapter save remains at its pre-wake state. */
BfmBedroom *bfm_bedroom_open_chapter(const MusashiDiscMedia *disc, const BfmSaveRecord *record,
                                     BfmGameplayRun *run);
/* Adopt the restored room flags into the process-owned bank after loading. */
void bfm_bedroom_bind_story_flags(BfmBedroom *room, uint8_t flags[64]);
void bfm_bedroom_bind_run(BfmBedroom *room, BfmGameplayRun *run);
void bfm_bedroom_free(BfmBedroom *room);
enum { BFM_BEDROOM_ADVANCED = 1, BFM_BEDROOM_VOICE_STOPPED };
int bfm_bedroom_step(BfmBedroom *room);
/* A stop result asks the desktop to cancel the queued voice output. */
/* Menu directions are digital. Field axes preserve stick deflection in -127..127. */
int bfm_bedroom_step_field(BfmBedroom *room, int confirm_edge, int reveal_edge,
                           int direction, int held, int accept_edge, int back_edge, BfmFieldInput field);
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
struct BfmInnField *bfm_bedroom_inn_counter(BfmBedroom *room);
int bfm_bedroom_leave_inn(BfmBedroom *room);
int bfm_bedroom_draw_counter(BfmBedroom *room, int dialogue, uint32_t pixels[240][320]);
int bfm_bedroom_draw(BfmBedroom *room, uint32_t pixels[240][320]);
/* Mounted field used for display, or zero when the location is unknown. */
uint16_t bfm_bedroom_display_room(const BfmBedroom *room);
typedef enum {
    BFM_ROOM_SCENE_BANK, BFM_ROOM_WOODS_BANK, BFM_ROOM_KNOCK_BANK, BFM_ROOM_RECOVERY_BANK,
    BFM_ROOM_BAKERY_BANK, BFM_ROOM_BOX_BANK, BFM_ROOM_GUIDE_BANK, BFM_ROOM_BANK_COUNT
} BfmRoomSoundBank;
typedef struct BfmBedroomSound {
    struct BfmBedroomEffects *effects;
    unsigned command;
    BfmRoomSoundBank bank;
} BfmBedroomSound;
/* Consume the current field once. The borrowed events last until the next
 * room update; their banks last for the lifetime of the room. */
size_t bfm_bedroom_take_effects(BfmBedroom *room, const BfmBedroomSound **events);
/* Peek at the last request, including after the desktop has consumed it. */
struct BfmBedroomEffects *bfm_bedroom_effect(const BfmBedroom *room, unsigned *command);
int bfm_bedroom_take_effect_release(BfmBedroom *room);
unsigned bfm_bedroom_take_score_command(BfmBedroom *room);
struct BfmForestScore *bfm_bedroom_score(const BfmBedroom *room);
/* A score command belongs to the just-completed field update. */
BfmRetryAudioEvent bfm_bedroom_retry_audio(BfmBedroom *room);
int bfm_bedroom_retry_score_result(BfmBedroom *room, BfmRetryAudioEvent event, int accepted);
/* Terminal recovery choice retained until the receiving scene can take over. */
BfmWoodsRetryPage bfm_bedroom_retry_destination(const BfmBedroom *room);
int bfm_bedroom_prepare_woods_recovery(BfmBedroom *room, BfmBedroomEntry *out);
struct BfmVillageImpactSound *bfm_bedroom_landing(const BfmBedroom *room);
struct BfmCableSound *bfm_bedroom_slider(const BfmBedroom *room, int *event);
int bfm_bedroom_endpoint(const BfmBedroom *room);
const char *bfm_bedroom_end_message(const BfmBedroom *room);
struct ForestVoice *bfm_bedroom_voice(BfmBedroom *room, unsigned *generation);
/* NTSC field of the latest mounted stream becoming ready on the room clock. */
unsigned bfm_bedroom_voice_ready_field(const BfmBedroom *room);
#endif
