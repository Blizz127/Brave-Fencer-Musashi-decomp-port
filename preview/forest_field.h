/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_FIELD_H
#define BFM_FOREST_FIELD_H

#include "musashi_forest_player_mesh.h"
#include "forest_player_state.h"
#include "forest_terrain.h"
#include "forest_dust.h"

typedef struct BfmForestField BfmForestField;

typedef struct {
    int16_t height;
    int16_t normal[3];
    uint16_t flags;
    uint16_t slope;
} BfmForestContact;

/* Build the terrain, floor and player-model owners for the actual Giant Forest room
 * selected by the scene loader. The disc may close after a successful call. */
BfmForestField *bfm_forest_field_open(const MusashiDiscMedia *disc, uint16_t room);
void bfm_forest_field_free(BfmForestField *field);
int bfm_forest_draw_ground(BfmForestField *field, const BfmForestCamera *camera,
                           uint32_t pixels[240][320]);
/* Composite the live actor and terrain in one depth order. The game supplies
 * its current model root, animation pose and equipped parts. */
int bfm_forest_draw_player(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, uint32_t pixels[240][320]);
/* The ordinary field renderer alternates 240-line VRAM draw buffers. Buffer
 * selection affects signed screen coordinates of nearby terrain faces. */
int bfm_forest_draw_player_buffer(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, unsigned buffer, uint32_t pixels[240][320]);
/* Update the room-owned bank actor after a completed player movement tick. */
/* Advance already emitted cards and, for a real player action, its new card. */
void bfm_forest_tick_dust(BfmForestField *field, BfmForestDustEmission event,
                          const int16_t player[3], const int32_t camera_eye[3], int yaw);
typedef struct {
    int connected;
    int16_t position[3], normal[3];
} BfmForestHit;
int bfm_forest_submit_guard_and_impact(BfmForestField *field, const BfmForestHit *hit,
                           const int32_t camera_eye[3], const BfmForestCamera *camera);
int bfm_forest_tick_shore_actor(BfmForestField *field, const int16_t player[3]);
/* Resolve the first sword impact using the current and next action key.
 * A hit turns the player and guard. Both world-space effect outputs are optional. */
int bfm_forest_first_strike(BfmForestField *field, const BfmForestTransform *player_root,
                            const int8_t tip[3], const int8_t next_tip[3], int16_t *yaw,
                            int16_t effect_position[3], int16_t effect_normal[3]);
int bfm_forest_draw_entrance_player(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *frame,
                           uint32_t pixels[240][320]);
const MusashiForestPlayerPart *bfm_forest_player_part(const BfmForestField *field,
                                                       unsigned part);

/* Apply the selected authored forest entrance to an existing player and its
 * current model. Fixed-point fractions and every other player field persist.
 * The final XYZ mirror belongs to the field owner, separate from model yaw. */
int bfm_forest_place_player(const BfmForestField *field, unsigned entrance,
                            ForestPlayerState *player, int16_t *model_yaw,
                            int16_t cached_position[3]);

/* Certify a short horizontal step in the same mounted cell using its flat
 * faces and signed wall planes. An unresolved contact stays with the caller. */
int bfm_forest_clear_flat_step(const BfmForestField *field, const int16_t from[3],
                               int16_t x, int16_t z);

/* Walk on the mounted flat floor polygons surrounding the released actor.
 * Bounded wall faces clip the horizontal endpoint, including successive
 * contacts at a corner. Unsupported contacts leave the endpoint untouched. */
int bfm_forest_doorway_step(const BfmForestField *field, const int16_t origin[3],
                            const int16_t from[3], int16_t to[3]);

typedef enum {
    BFM_FOREST_TOP_BLOCKED, BFM_FOREST_TOP_GROUNDED, BFM_FOREST_TOP_LEFT, BFM_FOREST_TOP_FREE
} BfmForestTopStep;

/* Take a supported step on the raised obstacle. On leaving its far edge,
 * return the final swept position above the landing floor to the fall owner.
 * An unresolved contact leaves the proposed endpoint untouched. */
BfmForestTopStep bfm_forest_raised_step(const BfmForestField *field,
    const BfmForestContact *previous, const int16_t from[3], int16_t to[3]);

/* When a stride misses the last raised face entirely, the falling action
 * owns this update from the original position. */
BfmForestTopStep bfm_forest_raised_motion(const BfmForestField *field,
    const BfmForestContact *previous, const int16_t from[3], int16_t to[3],
    const uint32_t start_pos[3], const uint32_t end_pos[3]);

/* Take a grounded step through the entrance and the adjoining slope.
 * The contact is the registered floor probe at the starting position. */
int bfm_forest_corridor_step(const BfmForestField *field, const BfmForestContact *previous,
                             const int16_t from[3], int16_t to[3]);

/* Run the original player's vertical terrain probe, on the field thread.
 * Pass signed game-world units. No hit leaves contact untouched. */
int bfm_forest_ground(const BfmForestField *field, int16_t x, int16_t y,
                      int16_t z, BfmForestContact *contact);
/* Resolve a short airborne step against the mounted walls and landing surface. */
int bfm_forest_air_step(const BfmForestField *field, const int16_t from[3],
                        int16_t to[3], BfmForestContact *contact, int *landed);
#endif
