/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_CORE_ORIGIN_H
#define BFM_STEAM_KNIGHT_CORE_ORIGIN_H
#include "steam_knight_machine.h"

/* Zero-based simulation updates since the held final knee pose and the open state. */
unsigned bfm_steam_knight_core_kneel_extension(unsigned held_updates, unsigned open_updates);
unsigned bfm_steam_knight_core_rise_extension(unsigned reverse_key);

/* Compose the live child origin from its chassis attachment and local travel.
 * The box itself contains the offset to its center. */
int bfm_steam_knight_core_transform(const BfmSteamKnightMachine *machine, int waist,
                                    unsigned extension, BfmForestTransform *world);
#endif
