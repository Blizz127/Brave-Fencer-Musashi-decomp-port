/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_FIGURE_H
#define BFM_BEDROOM_FIGURE_H
#include "bedroom_field.h"
#include "bedroom_travel.h"
#include "forest_dust.h"
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
    const int8_t *lumina_guard, *lumina_blade;
    unsigned blade_tick, green_blade;
} BfmBedroomBody;

typedef struct {
    int16_t axis[3];
    uint8_t rgb[3];
} BfmBedroomLight;

/* Replace the three directional lights for the whole figure queue. The room
 * ambient light is common to Musashi and the other actors in the field. */
int bfm_bedroom_figure_lights(BfmBedroomFigure *figure, const BfmBedroomLight lights[3]);

BfmBedroomFigure *bfm_bedroom_figure_load(const MusashiDiscMedia *disc,
                     const uint8_t *scene, size_t scene_bytes, uint32_t room_address,
                     uint32_t face_address, uint32_t shadow_address);
void bfm_bedroom_figure_free(BfmBedroomFigure *figure);
int bfm_bedroom_figure_prepare(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmBedroomBody *body, uint16_t tiles[512][1024], uint32_t *pixels);
/* Append the detached one-part sword and projected field quads to the prepared
 * figure queue, before its first draw. Both use the room's normal depth order. */
int bfm_bedroom_figure_add_fusion(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmForestActor *blade, const BfmForestDustCard *cards, size_t count);
/* Submit a one-part actor whose shape has already been evaluated. The
 * geometry and mounted shadow record are borrowed for this call. */
int bfm_bedroom_figure_add_morph(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const MusashiForestPlayerPart *part, const BfmBedroomRoot *root,
                     unsigned horizontal_q12, unsigned vertical_q12,
                     const uint8_t shadow[12], unsigned shadow_size);
/* Supply this callback to the field's ordering-table draw. */
int bfm_bedroom_figure_next_depth(const BfmBedroomFigure *figure);
void bfm_bedroom_figure_draw_to(void *figure, int depth);
#endif
