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
    int16_t lumina_scale_q12; /* Drawn sword mesh only. Zero uses normal size. */
} BfmBedroomBody;

typedef struct {
    int16_t axis[3];
    uint8_t rgb[3];
} BfmBedroomLight;

/* Replace the three directional lights for the whole figure queue. The room
 * ambient light is common to Musashi and the other actors in the field. */
int bfm_bedroom_figure_lights(BfmBedroomFigure *figure, const BfmBedroomLight lights[3]);
void bfm_bedroom_figure_model_lighting(BfmBedroomFigure *figure,
                                      const BfmForestModelLighting *lighting);

BfmBedroomFigure *bfm_bedroom_figure_load(const MusashiDiscMedia *disc,
                     const uint8_t *scene, size_t scene_bytes, uint32_t room_address,
                     uint32_t face_address, uint32_t shadow_address);
void bfm_bedroom_figure_free(BfmBedroomFigure *figure);
/* Musashi's world model transform includes the supporting surface and pose
 * pitch. Field attachments may use it even if the frame is not drawn. */
int bfm_bedroom_figure_actor_root(const BfmBedroomRoot *actor, unsigned pitch,
                                  BfmForestTransform *out);
int bfm_bedroom_figure_prepare(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmBedroomBody *body, uint16_t tiles[512][1024], uint32_t *pixels);
enum { BFM_HERO_NORMAL_SCALE = 4096, BFM_HERO_MINI_SCALE = 1365 };
/* World transform shared by a posed figure and its mounted effect anchors. */
int bfm_bedroom_figure_attachment_root(const BfmBedroomRoot *actor, unsigned pitch,
                                       unsigned body_scale_q12, const int animation_q12[3],
                                       BfmForestTransform *out);
/* The field selects the effective scale from the player's active status.
 * Body position stays fixed and the shadow retains its own floor contact. */
int bfm_bedroom_figure_prepare_scaled(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmBedroomBody *body, unsigned body_scale_q12,
                     uint16_t tiles[512][1024], uint32_t *pixels);
/* Independent pose scaling in model X, Y, Z. NULL uses unity on all axes. */
int bfm_bedroom_figure_prepare_axes(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmBedroomBody *body, unsigned body_scale_q12, const int animation_q12[3],
                     uint16_t tiles[512][1024], uint32_t *pixels);
/* Submit a detached one-part sword, projected field quads or two-vertex
 * additive lines while the room depth queue is still accepting geometry. */
int bfm_bedroom_figure_add_fusion(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmForestActor *blade, const BfmForestDustCard *cards, size_t count);
/* Append the visible parts of an independently posed room actor in the field's
 * depth order. The optional shadow uses the supplied ground projection. */
int bfm_bedroom_figure_add_actor(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmForestActor *actor, const BfmBedroomRoot *ground,
                     const uint8_t shadow[12], unsigned shadow_size);
/* Class-5 field models with the far-depth flag queue each lit, flat textured
 * quad at its farthest vertex. The untextured faces keep their average depth. */
int bfm_bedroom_figure_add_far_ft4_actor(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmForestActor *actor);
/* Unlit textured triangles and quads on the Valley gates use their farthest
 * vertex to order the two leaves against the surrounding cliffs. */
int bfm_bedroom_figure_add_far_unlit_actor(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmForestActor *actor);
/* Draw with the room lights and the specified translucency equation on each face. */
int bfm_bedroom_figure_add_translucent(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmForestActor *actor, unsigned blend);
/* These models supply their own three directional lights and force the same
 * translucency equation on every face. The room ambient still applies. */
int bfm_bedroom_figure_add_lit_translucent(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmForestActor *actor, const BfmBedroomLight lights[3], unsigned blend);
/* Submit a one-part actor whose shape has already been evaluated. The
 * geometry and mounted shadow record are borrowed for this call. */
int bfm_bedroom_figure_add_morph(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const MusashiForestPlayerPart *part, const BfmBedroomRoot *root,
                     unsigned horizontal_q12, unsigned vertical_q12,
                     const uint8_t shadow[12], unsigned shadow_size);
/* Supply this callback to the field's ordering-table draw. */
int bfm_bedroom_figure_next_depth(const BfmBedroomFigure *figure);
void bfm_bedroom_figure_draw_to(void *figure, int depth);
/* Independent floor sprite. At birth it uses xyz and a level pose. Later
 * fields use the parent's yaw and the last visible vertical floor contact. */
int bfm_bedroom_figure_add_floor_shadow(BfmBedroomFigure *figure, const BfmBedroomView *view,
    const BfmBedroomRoot *ground, const uint8_t texture[12], unsigned scale, int at_birth);

#endif
