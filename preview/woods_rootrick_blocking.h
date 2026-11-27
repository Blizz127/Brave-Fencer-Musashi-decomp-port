/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_BLOCKING_H
#define BFM_WOODS_ROOTRICK_BLOCKING_H

#include "bedroom_travel.h"
#include "village_camera.h"

typedef struct BfmWoodsRootrickBlocking BfmWoodsRootrickBlocking;

/* Borrow the mounted SC02/11 scene. Take over the ordinary player's current
 * root after Rootrick's bend trigger has actually fired. */
BfmWoodsRootrickBlocking *bfm_woods_rootrick_blocking_open(const uint8_t *scene, size_t bytes,
                                                          const BfmBedroomRoot *player);
void bfm_woods_rootrick_blocking_free(BfmWoodsRootrickBlocking *blocking);

/* Absolute field waypoints and looks come from the active actor script.
 * For a look at Rootrick, supply his actual position when the command fires. */
int bfm_woods_rootrick_walk(BfmWoodsRootrickBlocking *blocking, int16_t x, int16_t z);
int bfm_woods_rootrick_face(BfmWoodsRootrickBlocking *blocking, int16_t x, int16_t z);
int bfm_woods_rootrick_step_player(BfmWoodsRootrickBlocking *blocking,
                                   const BfmVillageCamera *previous_view);
const BfmBedroomRoot *bfm_woods_rootrick_player(const BfmWoodsRootrickBlocking *blocking);

const int64_t *bfm_woods_rootrick_player_position(const BfmWoodsRootrickBlocking *blocking);

/* Start a world-space camera cue supplied by this scene's field program.
 * Apply once per 30 Hz update following the ordinary camera. Clear the
 * fixed shot when the race camera takes ownership. */
int bfm_woods_rootrick_frame(BfmWoodsRootrickBlocking *blocking, uint32_t record_address,
                             const BfmVillageCamera *presented);
void bfm_woods_rootrick_step_camera(BfmWoodsRootrickBlocking *blocking, BfmVillageCamera *view);
void bfm_woods_rootrick_release_camera(BfmWoodsRootrickBlocking *blocking);

/* Rootrick's wall race switches to a close orbit around the player's
 * post-movement position. Begin presents the first cut immediately. */
int bfm_woods_rootrick_race_camera_begin(BfmWoodsRootrickBlocking *blocking,
                                         const int16_t foot[3], BfmVillageCamera *view);
int bfm_woods_rootrick_race_camera_step(BfmWoodsRootrickBlocking *blocking,
                                        const int16_t foot[3], BfmVillageCamera *view);

/* At the room-camera command, save the previous unshaken shot and restore the
 * ordinary room follow at the current player root. After each follow update
 * pass a copy of its base to step, including on this field. The return blends
 * only the drawn eye and target. Step returns 1 during its 48 field visits. */
void bfm_woods_rootrick_room_camera_begin(BfmWoodsRootrickBlocking *blocking,
                                         const BfmVillageCamera *previous);
int bfm_woods_rootrick_room_camera_step(BfmWoodsRootrickBlocking *blocking,
                                       BfmVillageCamera *room);

#endif
