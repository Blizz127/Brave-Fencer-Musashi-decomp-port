/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_VIEW_H
#define BFM_VILLAGE_MINE_VIEW_H

#include "village_field.h"

typedef struct BfmVillageMineView BfmVillageMineView;

/* The expanded mine scene and caller's tile pages stay mounted until free. */
BfmVillageMineView *bfm_village_mine_view_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024]);
void bfm_village_mine_view_free(BfmVillageMineView *view);
BfmVillageField *bfm_village_mine_view_field(BfmVillageMineView *view);
const BfmVillageCamera *bfm_village_mine_view_camera(const BfmVillageMineView *view);
/* Camera-relative input uses the direction from the submitted eye to its target. */
int bfm_village_mine_view_pad_yaw(const BfmVillageMineView *view);
void bfm_village_mine_view_follow(BfmVillageMineView *view, const int16_t foot[3], int entering);

/* The adjacent gallery uses Musashi's heading and either shoulder's press edge. */
BfmVillageMineView *bfm_village_mine_corridor_view_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024]);
void bfm_village_mine_corridor_view_follow(BfmVillageMineView *view, const int16_t foot[3],
    unsigned player_yaw, int entering, int shoulder_pressed);
/* Shared actor lamps and halo color after the room's lighting actor has ticked. */
void bfm_village_mine_corridor_glow(unsigned world_tick, uint8_t lights[3], uint8_t halo[3]);

int bfm_village_mine_view_draw(BfmVillageMineView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], unsigned age, unsigned scale_q12,
    uint32_t pixels[240][320],
    int (*submit)(void *, BfmBedroomFigure *, const BfmBedroomView *), void *actor);

#endif
