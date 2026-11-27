/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_BOX_VIEW_H
#define BFM_WOODS_BOX_VIEW_H
#include "village_field.h"

typedef struct BfmWoodsBoxView BfmWoodsBoxView;
struct BfmGameplayRun;

/* Borrows the SC02/11 scene and shared VRAM until free. */
BfmWoodsBoxView *bfm_woods_box_view_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, unsigned phase, uint16_t tiles[512][1024]);
void bfm_woods_box_view_free(BfmWoodsBoxView *view);
const BfmVillageCamera *bfm_woods_box_view_camera(const BfmWoodsBoxView *view);
void bfm_woods_box_view_follow(BfmWoodsBoxView *view, const int16_t foot[3],
                               int entering, int shoulder_pressed);
void bfm_woods_box_view_follow_grip(BfmWoodsBoxView *view, const int16_t foot[3],
                                    int entering, int shoulder_pressed, int gripped);
void bfm_woods_box_view_shake(BfmWoodsBoxView *view, int world_y_offset);
void bfm_woods_box_view_present(BfmWoodsBoxView *view, const BfmVillageCamera *camera);
void bfm_woods_box_view_light(BfmWoodsBoxView *view, unsigned phase);
/* Advance the existing splash once per player update. A new splash uses the
 * original contact point and the camera presented before this update. */
int bfm_woods_box_view_water_step(BfmWoodsBoxView *view, const int16_t splash_at[3],
                                  int hide_body, int hide_shadow, struct BfmGameplayRun *run);
/* The defeat presentation borrows the mounted field and its figure queue. */
BfmVillageField *bfm_woods_box_view_field(BfmWoodsBoxView *view);

/* The caller prepares its room actor in the shared depth-sorted figure queue. */
int bfm_woods_box_view_draw(BfmWoodsBoxView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], unsigned age, unsigned scale_q12,
    uint32_t pixels[240][320],
    int (*prepare_actor)(void *, BfmBedroomFigure *, const BfmBedroomView *), void *actor);
#endif
