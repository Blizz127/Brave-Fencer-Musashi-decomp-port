/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_FIELD_H
#define BFM_WOODS_EARTH_SHRINE_FIELD_H

#include "woods_earth_shrine_release.h"
#include "woods_box_view.h"
#include "village_arrival_motion.h"
#include "forest_message.h"

typedef struct BfmEarthShrineField BfmEarthShrineField;

BfmEarthShrineField *bfm_earth_shrine_field_open(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes);
void bfm_earth_shrine_field_free(BfmEarthShrineField *field);

/* Run before the child on every ceremony field. player_ready is the ordinary
 * player worker's availability, including weapon recovery and the award queue.
 * A busy external owner retains Musashi's pose and coordinates. */
int bfm_earth_shrine_field_player(BfmEarthShrineField *field, BfmVillageMotion *motion,
    BfmWoodsBoxView *view, struct BfmGameplayRun *run, int player_ready);

/* Route FIELD_COMMAND, PLAYER_READY, MESSAGE_CLOSED and CAMERA_READY here.
 * RELEASE_SHAKE also routes here before the next camera service.
 * Supply the currently focused opening slot. The caller owns each accepted
 * message and runs the ordinary field message/voice path until actual close.
 * Both speaker channels use the default offset 0 and left placement. */
int bfm_earth_shrine_field_event(BfmEarthShrineField *field, const BfmEarthShrineRelease *release,
    BfmEarthReleaseEvent event, unsigned kind, uint32_t operand, const uint8_t *data,
    size_t bytes, BfmWoodsBoxView *view, BfmForestMessage **opened);
/* Transfer the held dialogue pose to ordinary standing after RELEASED. */
int bfm_earth_shrine_field_finish(BfmEarthShrineField *field, BfmVillageMotion *motion);
int bfm_earth_shrine_field_closed(BfmEarthShrineField *field);
int bfm_earth_shrine_field_take_hud(BfmEarthShrineField *field);

/* Restart a mounted field shake at sample zero. Records contain signed
 * world-Y displacement and unsigned hold pairs, ending at a zero hold.
 * Keep the record available while it plays. */
int bfm_earth_shrine_field_shake(BfmEarthShrineField *field, const uint8_t *record, size_t bytes);

/* After commands and ordinary room follow, once per effective field. Keep
 * calling during the return blend after the child has released control. */
void bfm_earth_shrine_field_camera(BfmEarthShrineField *field, BfmWoodsBoxView *view);
int bfm_earth_shrine_field_draw_notice(const BfmEarthShrineField *field,
    BfmBedroomFigure *figure, const BfmBedroomView *view);
#endif
