/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_CORE_CONTACT_H
#define BFM_STEAM_KNIGHT_CORE_CONTACT_H

#include "forest_guard_contact.h"

typedef enum {
    BFM_STEAM_CLOSED_CAP,
    BFM_STEAM_OPEN_CORE
} BfmSteamKnightCoreSurface;

typedef struct {
    BfmForestGuardBox closed, opened;
} BfmSteamKnightCoreBoxes;

typedef struct {
    int16_t center[3], point[3], normal[3];
} BfmSteamKnightCoreContact;

int bfm_steam_knight_core_boxes_load(BfmSteamKnightCoreBoxes *boxes,
                                     const uint8_t *scene, size_t bytes);

/* Use the current child attachment transform and the active blade rays. */
int bfm_steam_knight_core_intersect(const BfmSteamKnightCoreBoxes *boxes,
                                    BfmSteamKnightCoreSurface surface,
                                    const BfmForestTransform *attachment,
                                    const int16_t start[3], const int16_t ends[][3],
                                    size_t count, BfmSteamKnightCoreContact *contact);

#endif
