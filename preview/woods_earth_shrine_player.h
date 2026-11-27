/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_PLAYER_H
#define BFM_WOODS_EARTH_SHRINE_PLAYER_H

#include "bedroom_figure.h"
#include "forest_message.h"
#include "gameplay_run.h"
#include "village_arrival_motion.h"

typedef struct BfmEarthShrinePlayer BfmEarthShrinePlayer;
typedef enum {
    BFM_EARTH_PLAYER_PRESENTATION_NONE,
    BFM_EARTH_PLAYER_PRESENTATION_ENTER,
    BFM_EARTH_PLAYER_PRESENTATION_RESTORE
} BfmEarthPlayerPresentation;

/* Borrow the mounted disc and SC02/11 scene for this visit. Assets load on
 * the real AWARD_ACTION descriptor, after the authored pre-award worker. */
BfmEarthShrinePlayer *bfm_earth_shrine_player_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes);
void bfm_earth_shrine_player_free(BfmEarthShrinePlayer *player);
int bfm_earth_shrine_player_begin(BfmEarthShrinePlayer *player, uint32_t descriptor);

/* Call once per player field, even during the focused award window. Supply
 * the last presented field camera. The caller owns each returned window and
 * notifies closed() only after its real final page has finished. */
int bfm_earth_shrine_player_step(BfmEarthShrinePlayer *player, BfmVillageMotion *motion,
    const BfmBedroomView *view, BfmGameplayRun *run, BfmSaveRecord *save,
    uint16_t tiles[512][1024], BfmForestMessage **opened);
int bfm_earth_shrine_player_closed(BfmEarthShrinePlayer *player);
int bfm_earth_shrine_player_active(const BfmEarthShrinePlayer *player);
int bfm_earth_shrine_player_complete(const BfmEarthShrinePlayer *player);

/* Apply to the ordinary hero pose, then submit effects in the shared figure.
 * Both reads are repeatable. The room retains its usual face tile upload. */
void bfm_earth_shrine_player_pose(const BfmEarthShrinePlayer *player, BfmBedroomPose *pose);
int bfm_earth_shrine_player_draw(const BfmEarthShrinePlayer *player,
    BfmBedroomFigure *figure, const BfmBedroomView *view);
/* Drain to zero each field. The stop-rings event has its own typed sound ID. */
unsigned bfm_earth_shrine_player_take_sound(BfmEarthShrinePlayer *player);
BfmEarthPlayerPresentation bfm_earth_shrine_player_take_presentation(BfmEarthShrinePlayer *player);

#endif
