/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_RESIDENTS_H
#define BFM_VILLAGE_RESIDENTS_H

#include "bedroom_figure.h"
#include "bedroom_record.h"
#include "forest_message.h"

typedef struct BfmVillageResidents BfmVillageResidents;

/* Earned File 1, hillside entrance 5. Keep the mounted SC03 member alive
 * until the residents are freed. The figure queue supplies the room lighting
 * and depth ordering shared with Musashi and the terrain. */
BfmVillageResidents *bfm_village_residents_load(const uint8_t *scene, size_t bytes,
                                                 uint32_t room, uint32_t shadow);
void bfm_village_residents_free(BfmVillageResidents *residents);

/* Call after preparing Musashi's figure for the current view. Age counts
 * village logic updates, independently of the number of presented frames. */
int bfm_village_residents_add(const BfmVillageResidents *residents, BfmBedroomFigure *figure,
                              const BfmBedroomView *view, unsigned age);
/* World transform of the active daylight station. */
int bfm_village_residents_station(const BfmVillageResidents *residents, int16_t xyz[3], unsigned *yaw);

/* Supply the focused Square edge only when the player has field control.
 * A successful approach claims the resident until the conversation ends. */
int bfm_village_residents_begin_talk(BfmVillageResidents *residents, const int16_t player[3],
                                     const BfmSaveRecord *state, int square_edge);
int bfm_village_residents_talking(const BfmVillageResidents *residents);
/* On the transition to true, dispatch the camera's release cue before the
 * current camera step. Control returns after that shot reaches its end. */
int bfm_village_residents_camera_returning(const BfmVillageResidents *residents);

/* Step once per logic update after admission. The caller owns the returned
 * message, borrows its mounted bytes, and reports its CLOSED event below.
 * Only an opened message changes the in-memory story flag and talk tally. */
int bfm_village_residents_step_talk(BfmVillageResidents *residents, BfmSaveRecord *state,
                                    unsigned age, BfmForestMessage **opened);
/* Latch the projected speaker and side once when the field window opens.
 * The coordinates feed bfm_bedroom_message_art_draw_side. */
int bfm_village_residents_place_window(const BfmVillageResidents *residents,
                                       const BfmBedroomView *view, const int16_t player[3],
                                       int *anchor, int *portrait_on_right);
void bfm_village_residents_close_talk(BfmVillageResidents *residents);
#endif
