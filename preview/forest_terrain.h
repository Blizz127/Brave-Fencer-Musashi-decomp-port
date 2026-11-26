/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_TERRAIN_H
#define BFM_FOREST_TERRAIN_H

#include "forest_blade.h"

typedef struct BfmForestTerrain BfmForestTerrain;

/* World-to-view matrix, in the field camera's 12-bit fractional convention.
 * Translation is in game-world units. Projection is in original screen pixels. */
typedef struct {
    int16_t rotation[3][3];
    int32_t translation[3];
    uint16_t projection;
} BfmForestCamera;

/* Build the nearby, unparented zero-twist field view from the game camera's
 * current eye, target and focal length. Failure preserves the old view. */
int bfm_forest_camera_look_at(const int32_t eye[3], const int32_t target[3],
                             uint16_t projection, BfmForestCamera *out);

/* Compose a local effect with the same camera transform as a field model. */
int bfm_forest_effect_camera(const BfmForestCamera *camera, const BfmForestTransform *model,
                             BfmForestCamera *local);

BfmForestTerrain *bfm_forest_terrain_load(const MusashiDiscMedia *disc,
                                          const uint8_t *mesh, size_t bytes);
BfmForestTerrain *bfm_forest_terrain_load_member(const MusashiDiscMedia *disc,
                                          unsigned member, const uint8_t *mesh, size_t bytes);
/* Set the active room descriptor's ambient and directional lights. */
int bfm_forest_terrain_set_lighting(BfmForestTerrain *terrain,
                                    const uint8_t *descriptor, size_t bytes);
/* Borrow the mounted room graphics until the terrain is released. */
const uint16_t (*bfm_forest_terrain_tiles(const BfmForestTerrain *terrain))[1024];
void bfm_forest_terrain_free(BfmForestTerrain *terrain);

/* Draw the room onto the caller's existing ARGB8888 320-by-240 screen.
 * Use the frame's field camera. Geometry and palettes belong to the mounted
 * room. The caller composes scene effects and the HUD. */
int bfm_forest_terrain_draw(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                            uint32_t pixels[240][320]);
int bfm_forest_terrain_draw_actor(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *mesh, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t visible_parts,
                                  uint32_t pixels[240][320]);
int bfm_forest_terrain_draw_actor_buffer(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *mesh, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t visible_parts, unsigned buffer,
                                  uint32_t pixels[240][320]);
int bfm_forest_terrain_draw_entrance(BfmForestTerrain *terrain, const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *mesh, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t visible_parts,
                                  const BfmForestBlade *blade, unsigned buffer, const BfmForestBladeFrame *frame,
                                  uint32_t pixels[240][320]);
/* A second room-mounted model shares the player's field camera, lighting and
 * depth ordering. Mesh, root and local transforms are borrowed for this draw. */
typedef struct {
    const MusashiForestPlayerMesh *mesh;
    const BfmForestPose *pose;
    const BfmForestTransform *root;
    uint32_t visible_parts;
    /* A buffered submission owns the camera that produced its screen coordinates. */
    const BfmForestCamera *submitted_camera;
} BfmForestActor;

int bfm_forest_terrain_draw_entrance_with_actor(BfmForestTerrain *terrain,
                                  const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *player, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t visible_parts,
                                  const BfmForestBlade *blade, unsigned buffer,
                                  const BfmForestBladeFrame *frame, const BfmForestActor *actor,
                                  uint32_t pixels[240][320]);
/* The forest actor pool has 96 slots. All supplied models share one depth sort. */
enum { BFM_FOREST_ACTOR_LIMIT = 96 };
int bfm_forest_terrain_draw_entrance_with_actors(BfmForestTerrain *terrain,
                                  const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *player, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t visible_parts,
                                  const BfmForestBlade *blade, unsigned buffer,
                                  const BfmForestBladeFrame *frame, const BfmForestActor *actors,
                                  size_t actor_count, uint32_t pixels[240][320]);
struct BfmForestDustCard;
int bfm_forest_terrain_draw_entrance_with_effects(BfmForestTerrain *terrain,
                                  const BfmForestCamera *camera,
                                  const MusashiForestPlayerMesh *player, const BfmForestPose *pose,
                                  const BfmForestTransform *root, uint32_t visible_parts,
                                  const BfmForestBlade *blade, unsigned buffer,
                                  const BfmForestBladeFrame *frame, const BfmForestActor *actors,
                                  size_t actor_count, const struct BfmForestDustCard *cards,
                                  size_t card_count, uint32_t pixels[240][320]);
#endif
