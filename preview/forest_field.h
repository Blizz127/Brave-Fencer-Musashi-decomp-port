/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_FIELD_H
#define BFM_FOREST_FIELD_H

#include "musashi_forest_player_mesh.h"
#include "forest_player_state.h"
#include "forest_terrain.h"
#include "forest_dust.h"
#include "gameplay_run.h"
#include "room_descriptor.h"

typedef struct BfmForestField BfmForestField;

/* Update the distance-activated exit actor. After a committed movement step,
 * report its authored transfer and separate entrance selector once. */
void bfm_forest_tick_gateway(BfmForestField *field, const int16_t player[3]);
int bfm_forest_cross_gateway(BfmForestField *field, const int16_t before[3],
                             const int16_t after[3], RoomTransferDescriptor *destination,
                             uint16_t *entry);

typedef struct {
    int16_t height;
    int16_t normal[3];
    uint16_t flags;
    uint16_t slope;
    unsigned bridge;
} BfmForestContact;

typedef enum {
    BFM_SHORE_FLOOR_UNKNOWN, BFM_SHORE_FLOOR_FLAT, BFM_SHORE_FLOOR_GRADE, BFM_SHORE_FLOOR_STEEP
} BfmForestShoreFloor;
/* Resolve the coastal patrol's two ground probes and any intervening wall.
 * Flat and grade results replace whole coordinates, retaining caller-owned
 * fractions. Steep and unknown contacts leave the proposed step intact. */
BfmForestShoreFloor bfm_forest_shore_floor_step(const BfmForestField *field,
                                                const int16_t from[3], int16_t to[3]);

/* Clip a mounted short stride along the bounded vertical faces before probing support. */
void bfm_forest_clip_field_walls(const BfmForestField *field, const int16_t from[3], int16_t to[3]);
int bfm_forest_escape_floor_step(const BfmForestField *field, const int16_t from[3], int16_t to[3]);
BfmForestShoreFloor bfm_forest_pursuit_floor_step(const BfmForestField *field,
                                                  const int16_t from[3], int16_t to[3]);

/* Build the terrain, floor and player-model owners for the mounted field room
 * selected by the scene loader. The disc may close after a successful call. */
BfmForestField *bfm_forest_field_open(const MusashiDiscMedia *disc, uint16_t room);
/* Mount only a room's collision grid for ordinary hero movement. The caller owns
 * both the room's rendering and its actor pool. Release with bfm_forest_field_free. */
BfmForestField *bfm_forest_motion_surface_open(const uint8_t *scene, size_t bytes, uint32_t room);
void bfm_forest_field_free(BfmForestField *field);
/* Borrow this room's unpacked scene and 36-byte authored doorway. */
int bfm_forest_mount_summit_road(BfmForestField *field, const MusashiDiscMedia *disc);
const uint8_t *bfm_forest_field_scene(const BfmForestField *field, size_t *size);
const uint16_t (*bfm_forest_field_tiles(const BfmForestField *field))[1024];
const uint8_t *bfm_forest_field_entrance(const BfmForestField *field, unsigned index);
int bfm_forest_draw_room(const BfmForestField *field, const BfmForestCamera *camera,
                         const BfmForestPose *pose, const BfmForestTransform *root,
                         uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *blade,
                         const BfmForestActor *actors, size_t actor_count, const BfmForestDustCard *cards, size_t card_count,
                         uint32_t pixels[240][320]);
int bfm_forest_draw_ground(BfmForestField *field, const BfmForestCamera *camera,
                           uint32_t pixels[240][320]);
/* Composite the live actor and terrain in one depth order. The game supplies
 * its current model root, animation pose and equipped parts. */
int bfm_forest_draw_player(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, uint32_t pixels[240][320]);
/* A moving player already inside a soldier keeps his stride while bracing. */
int bfm_forest_melee_brace(const BfmForestField *field, const int16_t from[3]);
/* The ordinary field renderer alternates 240-line VRAM draw buffers. Buffer
 * selection affects signed screen coordinates of nearby terrain faces. */
int bfm_forest_draw_player_only(const BfmForestField *field, const BfmForestCamera *camera,
                                const BfmForestPose *pose, const BfmForestTransform *root,
                                uint32_t mask, const BfmForestModelLighting *lighting, uint32_t pixels[240][320]);
int bfm_forest_draw_player_buffer(BfmForestField *field, const BfmForestCamera *camera,
                           const BfmForestPose *pose, const BfmForestTransform *root,
                           uint32_t visible_parts, unsigned buffer, uint32_t pixels[240][320]);
/* Start permanent ambient placements on the first forest world update. */
void bfm_forest_start_ambient(BfmForestField *field, BfmGameplayRun *run);
void bfm_forest_submit_atmosphere(BfmForestField *field, const BfmForestCamera *camera,
                                  const int32_t eye[3]);
/* Advance already emitted cards and, for a real player action, its new card. */
void bfm_forest_tick_dust(BfmForestField *field, BfmForestDustEmission event,
                          const int16_t player[3], const int32_t camera_eye[3], int yaw);
enum { BFM_FOREST_STRIKE_RAYS = 3 };
typedef struct {
    int connected;
    uint16_t strength;
    int16_t position[3], normal[3];
} BfmForestHit;
int bfm_forest_tick_sweep(BfmForestField *field, const MusashiDiscMedia *disc,
                          int begin, uint32_t logical_pose, const BfmForestTransform *previous_root);
/* Queue a confirmed loss at the unshifted player root. Its effect draws randomness on dispatch. */
int bfm_forest_queue_hurt_number(BfmForestField *field, const int16_t player[3], uint16_t amount);
void bfm_forest_begin_water_effect(BfmForestField *field, const int16_t foot[3],
                                   const int32_t eye[3], BfmGameplayRun *run);
int bfm_forest_submit_guard_and_impact(BfmForestField *field, const BfmForestHit *hit,
                           const int32_t camera_eye[3], const BfmForestCamera *camera);
int bfm_forest_submit_guard_and_impact_with_run(BfmForestField *field, const BfmForestHit *hit,
                           const int32_t camera_eye[3], const BfmForestCamera *camera,
                           BfmGameplayRun *run);
/* Submit accepted ray contacts in their original order, then advance all existing field effects. */
int bfm_forest_submit_guard_and_impacts_with_run(BfmForestField *field,
                           const BfmForestHit *hits, size_t count, const int32_t camera_eye[3],
                           const BfmForestCamera *camera, BfmGameplayRun *run);
int bfm_forest_tick_shore_actor_with_run(BfmForestField *field, const int16_t player[3],
                                        BfmGameplayRun *run);
int bfm_forest_tick_shore_actor_aim(BfmForestField *field, const int16_t player[3],
                                   const int16_t guard_target[3], BfmGameplayRun *run);
/* Advance the room controller and its registered logs before this update's
 * projectiles. A shot updates only the bodies its world-space segment hits. */
unsigned bfm_forest_field_bridge_step(BfmForestField *field, unsigned room_phase);
/* Both contact outputs are written only on a hit. The Q12 normal is suitable
 * for orienting an impact effect at the clipped endpoint. */
int bfm_forest_bridge_impact(void *field, const int16_t from[3], const int16_t to[3],
                             int16_t hit[3], int16_t normal[3]);
int bfm_forest_bridge_shot(void *field, const int16_t from[3], const int16_t to[3],
                           int16_t hit[3]);
int bfm_forest_tick_shore_actor(BfmForestField *field, const int16_t player[3]);
/* A solid face can separate a moving segment's endpoint from the wall while
 * preserving the tangential part of its movement. */
unsigned bfm_forest_resolve_solid_segment(const BfmForestField *field, const int16_t from[3],
                                          const int16_t to[3], int16_t contact[3]);

/* Consume the pending soldier or rifle hit. The output is the incoming world-space yaw. */
unsigned bfm_forest_take_player_strike(BfmForestField *field, int16_t *yaw);
int bfm_forest_take_player_hit(BfmForestField *field, int16_t *shot_yaw);
/* Resolve the first sword impact using the current and next action key.
 * A hit turns the player and guard. Both world-space effect outputs are optional. */
int bfm_forest_first_strike(BfmForestField *field, const BfmForestTransform *player_root,
                            const int8_t tip[3], const int8_t next_tip[3], int16_t *yaw,
                            int16_t effect_position[3], int16_t effect_normal[3]);
/* Apply a grounded Fusion swing to the placed shore soldier and bank riflemen.
 * Recovered bodies accept nonlethal hits. A miss preserves the outputs. A hit
 * returns the final contact and turns the attacker toward its body center. */
int bfm_forest_ground_strike(BfmForestField *field, const BfmForestTransform *player_root,
                             const int8_t tip[3], const int8_t next_tip[3], uint16_t strength,
                             int16_t *yaw, int16_t effect_position[3], int16_t effect_normal[3]);
/* Ordinary Fusion uses kinds 1 and 2 on alternating attacks. A target
 * suppresses the same kind during recoil and can accept a different kind. */
int bfm_forest_typed_ground_strike(BfmForestField *field, const BfmForestTransform *player_root,
                                  const int8_t tip[3], const int8_t next_tip[3],
                                  uint16_t strength, uint16_t attack_type, int16_t *yaw,
                                  int16_t effect_position[3], int16_t effect_normal[3]);
/* Build the grounded slash's ordered world-space rays from adjacent motion keys. */
size_t bfm_forest_blade_rays(const BfmForestTransform *root, const int8_t tip[3],
                             const int8_t next_tip[3], int16_t start[3],
                             int16_t ends[BFM_FOREST_STRIKE_RAYS][3]);
/* Return the last accepted surface on each blade ray in issuance order. */
size_t bfm_forest_ground_strike_rays(BfmForestField *field, const BfmForestTransform *player_root,
                                    const int8_t tip[3], const int8_t next_tip[3],
                                    uint16_t strength, uint16_t attack_type, int16_t *yaw,
                                    BfmForestHit hits[BFM_FOREST_STRIKE_RAYS]);
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

/* Continue a stride across a supporting room actor. The falling action takes
 * the first unsupported stride when the foot clears the rim. */
BfmForestTopStep bfm_forest_step_on_bridge(const BfmForestField *field,
                                           const int16_t from[3], int16_t to[3]);

/* Whether this mounted floor can start or land an ordinary player jump. */
int bfm_forest_jump_surface(const BfmForestContact *ground);

/* Take a grounded step through the entrance, uphill trail and clearing.
 * The contact is the registered floor probe at the starting position. */
int bfm_forest_corridor_step(const BfmForestField *field, const BfmForestContact *previous,
                             const int16_t from[3], int16_t to[3]);

/* Read the body's downward support contact on the open creek bank. */
int bfm_forest_bank_body_normal(const BfmForestField *field, const int16_t from[3],
                               const int16_t to[3], int16_t normal[3]);
/* Run the original player's vertical terrain probe, on the field thread.
 * Pass signed game-world units. No hit leaves contact untouched. */
int bfm_forest_ground(const BfmForestField *field, int16_t x, int16_t y,
                      int16_t z, BfmForestContact *contact);
/* Sweep an airborne target along its path, or follow its support after
 * landing. Returns zero on unsupported terrain with its outputs intact. */
int bfm_forest_escape_head_probe(const BfmForestField *field, const int16_t from[3],
                                 int16_t to[3], int follow_ground, int *grounded);
int bfm_forest_thrown_probe(const BfmForestField *field, const int16_t from[3],
                            int16_t to[3], int follow_ground, int *grounded);
/* A remembered recovery foot must lie on the actual level bank. */
int bfm_forest_dry_creek_bank(const BfmForestField *field, const int16_t foot[3]);
/* Resolve the level stride that first leaves the supported near bank. */
int bfm_forest_walks_off_creek_bank(const BfmForestField *field, const int16_t from[3],
                                    const int16_t end[3]);
/* A shallow entry projects the dry stride onto the first near grade (2).
 * A deeper stride walks off level support (1). Other paths return zero. */
int bfm_forest_creek_bank_stride(const BfmForestField *field, const int16_t from[3], int16_t end[3]);
/* Acquire only the downward, bounded near-grade contact of a failed air sweep. */
int bfm_forest_creek_slope_contact(const BfmForestField *field, const int16_t from[3], int16_t end[3]);
/* Continue a supported slide (1), or leave its finite toe for air (2).
 * Both report the incline's Z normal if requested. Only 1 changes the foot. */
int bfm_forest_creek_slope_step(const BfmForestField *field, const int16_t from[3],
                                int16_t end[3], int16_t *normal_z);
/* The hero can additionally enter the bounded first-creek water surface. */
int bfm_forest_player_air_step(const BfmForestField *field, const int16_t from[3],
                               int16_t to[3], BfmForestContact *contact, int *landed, int *water);
/* Resolve a short airborne step against the mounted walls and landing surface. */
int bfm_forest_air_step(const BfmForestField *field, const int16_t from[3],
                        int16_t to[3], BfmForestContact *contact, int *landed);
#endif
