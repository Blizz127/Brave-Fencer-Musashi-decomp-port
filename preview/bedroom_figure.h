/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_FIGURE_H
#define BFM_BEDROOM_FIGURE_H
#include "bedroom_field.h"
#include "forest_blade.h"
#include "musashi_forest_player_mesh.h"

typedef struct BfmBedroomFigure BfmBedroomFigure;

typedef struct {
    const uint8_t *pose;          /* Motion packet including its 16-byte prefix. */
    size_t pose_bytes;
    int position[3], heading, pitch;
    int surface_normal[3];       /* Supporting face Q12 normal, with upward Y negative. */
    int shadow_position[3], shadow_normal[3], shadow_visible;
    uint32_t visible_parts, face_word;
    const BfmForestBlade *blade;
    unsigned blade_tick, green_blade;
} BfmBedroomBody;

BfmBedroomFigure *bfm_bedroom_figure_load(const MusashiDiscMedia *disc,
                     const uint8_t *scene, size_t scene_bytes, uint32_t room_address,
                     uint32_t face_address, uint32_t shadow_address);
void bfm_bedroom_figure_free(BfmBedroomFigure *figure);
int bfm_bedroom_figure_prepare(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmBedroomBody *body, uint16_t tiles[512][1024], uint32_t *pixels);
/* Supply this callback to the field's ordering-table draw. */
void bfm_bedroom_figure_draw_to(void *figure, int depth);
#endif
