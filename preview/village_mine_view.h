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

int bfm_village_mine_view_draw(BfmVillageMineView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], unsigned age, unsigned scale_q12,
    uint32_t pixels[240][320],
    int (*submit)(void *, BfmBedroomFigure *, const BfmBedroomView *), void *actor);

#endif
