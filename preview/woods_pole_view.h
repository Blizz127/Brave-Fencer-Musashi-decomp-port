/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_POLE_VIEW_H
#define BFM_WOODS_POLE_VIEW_H
#include "village_camera.h"
#include <stddef.h>

typedef struct {
    int focal, radius, pitch, yaw, target_offset[3], max_y;
    int player_y, pitch_now, yaw_now, offset_now[3];
    unsigned clear_ticks, has_reference;
    BfmVillageCamera saved;
} BfmWoodsPoleView;

/* Mount the camera actor in the SC02/11 placement list. State is per visit. */
int bfm_woods_pole_view_load(BfmWoodsPoleView *pole, const uint8_t *scene, size_t bytes);

/* Call once after player movement, including normal-room and restore updates. */
void bfm_woods_pole_view_reference(BfmWoodsPoleView *pole, const int16_t foot[3],
                                   int entering, int16_t reference[3]);
int bfm_woods_pole_view_active(const BfmWoodsPoleView *pole);

/* While inactive, update the ordinary camera before calling this. During the
 * close view, leave the ordinary orbit state frozen and call this alone.
 * The gripped signal comes from the current player's actual climb action. */
void bfm_woods_pole_view_step(BfmWoodsPoleView *pole, int gripped,
                              const int16_t foot[3], const int16_t reference[3],
                              int ordinary_pitch, int ordinary_offset_y, BfmVillageCamera *camera);
#endif
