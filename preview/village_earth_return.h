/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_EARTH_RETURN_H
#define BFM_VILLAGE_EARTH_RETURN_H

#include "bedroom_figure.h"
#include "forest_message.h"
#include "gameplay_run.h"
#include "village_arrival_motion.h"
#include "village_camera.h"

typedef struct BfmVillageEarthReturn BfmVillageEarthReturn;

int bfm_village_earth_return_pending(const BfmSaveRecord *save);
BfmVillageEarthReturn *bfm_village_earth_return_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    BfmSaveRecord *save);
void bfm_village_earth_return_free(BfmVillageEarthReturn *event);
/* Step from room registration, including the normal incoming reveal.
 * Readiness is the ordinary player's ability to take the registered event.
 * Results are 0 on error, 1 for free movement, 2 for a scripted field. */
int bfm_village_earth_return_step(BfmVillageEarthReturn *event, BfmVillageMotion *motion,
    BfmVillageCamera *camera, BfmGameplayRun *run, int player_ready, BfmForestMessage **opened);
int bfm_village_earth_return_active(const BfmVillageEarthReturn *event);
int bfm_village_earth_return_message_closed(BfmVillageEarthReturn *event);
int bfm_village_earth_return_take_hud(BfmVillageEarthReturn *event);
int bfm_village_earth_return_add(const BfmVillageEarthReturn *event,
    BfmBedroomFigure *figure, const BfmBedroomView *view);

int bfm_village_earth_return_load_art(BfmVillageEarthReturn *event,
    const MusashiDiscMedia *disc, uint16_t tiles[512][1024]);
int bfm_village_earth_return_draw_window(const BfmVillageEarthReturn *event,
    const BfmBedroomView *view, const BfmForestMessageFrame *frame, uint32_t pixels[240][320]);
unsigned bfm_village_earth_return_score(const BfmVillageEarthReturn *event, unsigned ordinary);
void bfm_village_earth_return_score_applied(BfmVillageEarthReturn *event, unsigned selector);
unsigned bfm_village_earth_return_take_sound(BfmVillageEarthReturn *event);

#endif
