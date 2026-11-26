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
int bfm_steam_knight_machine_joint(const BfmSteamKnightMachine *machine, unsigned chassis_part,
                                  BfmForestTransform *world, const MusashiForestPlayerPart **mesh_part);
#endif
