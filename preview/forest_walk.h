/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_WALK_H
#define BFM_FOREST_WALK_H
#include "forest_entry.h"
#include "gameplay_run.h"

typedef struct BfmForestWalk BfmForestWalk;

enum { BFM_FOREST_R1 = 1u << 11, BFM_FOREST_TRIANGLE = 1u << 12, BFM_FOREST_CIRCLE = 1u << 13, BFM_FOREST_CROSS = 1u << 14, BFM_FOREST_SQUARE = 1u << 15 };

/* The door owns this short automatic walk. Caller hands over the transfer
 * resolved from its active opening scene when the chapter card has finished. */
BfmForestWalk *bfm_forest_walk_open(const MusashiDiscMedia *disc, const RoomTransferDescriptor *entry);
/* Attach the application-owned gameplay stream before player control starts. */
int bfm_forest_walk_bind_gameplay(BfmForestWalk *walk, BfmGameplayRun *run);
/* Completed world updates at the first forest pose. A caller without the
 * departing scene's clock can still use action-triggered effects. */
void bfm_forest_walk_inherit_clock(BfmForestWalk *walk, uint16_t first_tick);
void bfm_forest_walk_free(BfmForestWalk *walk);
/* 1 advances one field update, 0 stops at the next unsupported scene event,
 * -1 indicates bad mounted data. The first frame can be drawn immediately. */
int bfm_forest_walk_tick(BfmForestWalk *walk);
int bfm_forest_walk_tick_input(BfmForestWalk *walk, int confirm_edge);
int bfm_forest_walk_tick_pad(BfmForestWalk *walk, int x, int y,
                             uint16_t pressed, uint16_t held);
int bfm_forest_walk_tick_controller(BfmForestWalk *walk, int x, int y, uint16_t pressed,
                                    uint16_t held, int stick_direction);
int bfm_forest_walk_tick_axes(BfmForestWalk *walk, int x, int y, int confirm_edge);
/* Cross is an edge for dialogue and a button with a duration during a jump. */
int bfm_forest_walk_tick_controls(BfmForestWalk *walk, int x, int y,
                                  int cross_pressed, int cross_held);
/* 0 during the door event, 1 under player control, 2 at the preview boundary. */
int bfm_forest_walk_control(const BfmForestWalk *walk);
/* True on the update that requests the grounded Fusion sound. */
int bfm_forest_walk_gunshot_sound(const BfmForestWalk *walk);
int bfm_forest_walk_swing_sound(const BfmForestWalk *walk);
int bfm_forest_walk_hit_sound(const BfmForestWalk *walk);
struct ForestVoice;
struct ForestVoice *bfm_forest_walk_voice(BfmForestWalk *walk, unsigned *generation);
int bfm_forest_walk_draw(BfmForestWalk *walk, uint32_t pixels[240][320]);
#endif
