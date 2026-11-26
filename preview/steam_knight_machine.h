/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_MACHINE_H
#define BFM_STEAM_KNIGHT_MACHINE_H
#include "forest_terrain.h"

typedef struct BfmSteamKnightMachine BfmSteamKnightMachine;

BfmSteamKnightMachine *bfm_steam_knight_machine_load(const uint8_t *scene, size_t bytes);
void bfm_steam_knight_machine_free(BfmSteamKnightMachine *machine);
/* Elapsed world updates from the ceiling cue, then from handover to Musashi. */
void bfm_steam_knight_machine_present(BfmSteamKnightMachine *machine, unsigned elapsed);
/* Enable submissions after the room has committed its roof-opening mesh. */
void bfm_steam_knight_machine_reveal(BfmSteamKnightMachine *machine);
void bfm_steam_knight_machine_handover(BfmSteamKnightMachine *machine);
size_t bfm_steam_knight_machine_actors(const BfmSteamKnightMachine *machine,
                                      BfmForestActor *out, size_t capacity);
/* The encounter selects an action once its live control has issued it. */
int bfm_steam_knight_machine_set_chassis_clip(BfmSteamKnightMachine *machine,
                                             unsigned clip, unsigned elapsed);
/* The encounter supplies gameplay results for a real inner leg break and
 * completion of its resulting weak-core opportunity. */
typedef enum {
    BFM_STEAM_MACHINE_DORMANT, BFM_STEAM_MACHINE_NEUTRAL, BFM_STEAM_MACHINE_WALKING,
    BFM_STEAM_MACHINE_KNEELING, BFM_STEAM_MACHINE_GETTING_UP,
    BFM_STEAM_MACHINE_STRIKING, BFM_STEAM_MACHINE_SMASHING, BFM_STEAM_MACHINE_TRANSFERRING
} BfmSteamKnightStance;
BfmSteamKnightStance bfm_steam_knight_machine_stance(const BfmSteamKnightMachine *machine);
int bfm_steam_knight_machine_world_root(const BfmSteamKnightMachine *machine, int16_t position[3]);
int bfm_steam_knight_machine_set_world_root(BfmSteamKnightMachine *machine, const int16_t position[3]);
void bfm_steam_knight_machine_track_hero(BfmSteamKnightMachine *machine, const int16_t feet[3]);
int bfm_steam_knight_machine_outer_broken(BfmSteamKnightMachine *machine, unsigned joint);
int bfm_steam_knight_machine_inner_broken(BfmSteamKnightMachine *machine, unsigned joint);
int bfm_steam_knight_machine_recover(BfmSteamKnightMachine *machine);
typedef enum {
    BFM_STEAM_WALK_RIGHT, BFM_STEAM_WALK_LEFT, BFM_STEAM_WALK_FORWARD
} BfmSteamKnightWalk;
int bfm_steam_knight_machine_walk(BfmSteamKnightMachine *machine, BfmSteamKnightWalk direction);
/* World-space movement and field progression come from the encounter. */
int bfm_steam_knight_machine_root_q12(const BfmSteamKnightMachine *machine, int32_t position[3]);
int bfm_steam_knight_machine_round_pose(BfmSteamKnightMachine *machine, unsigned stage,
                                       unsigned round, unsigned active_age);
int bfm_steam_knight_machine_reset_round(BfmSteamKnightMachine *machine, unsigned round);
enum { BFM_STEAM_WEAK_HALO_CARDS = 12 };
int bfm_steam_knight_machine_weak_tip(const BfmForestTransform *child, int16_t world[3]);
size_t bfm_steam_knight_machine_weak_halo(const BfmForestTransform *child,
    const BfmForestCamera *camera, struct BfmForestDustCard *cards, size_t capacity);
/* The detached front cover no longer occupies its original chassis joint. */
void bfm_steam_knight_machine_open_cover(BfmSteamKnightMachine *machine);
/* Sample the physical knee key before the display delay. hold_age starts
 * at zero when the terminal key is first selected. */
int bfm_steam_knight_machine_knee_pose(const BfmSteamKnightMachine *machine,
                                      unsigned *key, unsigned *hold_age, int *recovering);
int bfm_steam_knight_machine_core_socket(const BfmSteamKnightMachine *machine,
                                         int waist, BfmForestTransform *world);
/* Start the mounted outer shell's observed white response on actual damage. */
void bfm_steam_knight_machine_flash_armor(BfmSteamKnightMachine *machine, unsigned joint);
int bfm_steam_knight_machine_joint(const BfmSteamKnightMachine *machine, unsigned chassis_part,
                                  BfmForestTransform *world, const MusashiForestPlayerPart **mesh_part);
#endif
