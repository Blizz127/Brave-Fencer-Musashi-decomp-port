/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_FLAIL_H
#define BFM_STEAM_KNIGHT_FLAIL_H
#include "steam_knight_street_route.h"
#include "forest_pose.h"
#include "forest_guard_contact.h"

typedef struct {
    int32_t position[3], velocity[3]; /* signed 16.16 world units */
    int marker, active;
} BfmSteamKnightFlail;

typedef struct {
    BfmForestGuardBox hero, head;
} BfmSteamFlailContacts;

int bfm_steam_flail_contacts_load(BfmSteamFlailContacts *out, const uint8_t *scene, size_t bytes);
/* Test the two tiers of four world-space segments on a free shot against
 * the actual player model. The chosen mark bypasses the player service. */
int bfm_steam_flail_player_hit(const BfmSteamFlailContacts *contacts, int marker,
                              const int16_t position[3], const BfmForestTransform *hero,
                              int16_t contact[3], int16_t normal[3]);

typedef enum {
    BFM_STEAM_FLAIL_INVALID, BFM_STEAM_FLAIL_MOVING, BFM_STEAM_FLAIL_GROUNDED, BFM_STEAM_FLAIL_AT_MARKER
} BfmSteamFlailStep;

/* Wind the head around the moving attachment. Reach and yaw are initialized
 * when the body starts its attack. The orbit uses the world vertical axis. */
int bfm_steam_flail_wind(const BfmForestTransform *socket, unsigned *reach,
                         unsigned *angle, BfmForestTransform *head);
/* Return to the live attachment in four steps after the cable has tightened.
 * remaining is the quarter-weight of the endpoint, 3, 2, 1, then 0. */
int bfm_steam_flail_reel(const BfmForestTransform *socket, const int16_t endpoint[3],
                         unsigned remaining, BfmForestTransform *head);

/* The child latches its moving head after aim, then travels independently. */
int bfm_steam_flail_launch(BfmSteamKnightFlail *flail, const int16_t head[3],
    int16_t body_y, const int16_t player[3], const BfmSteamKnightStreetRoute *route,
    int marker, uint32_t body_flags);
BfmSteamFlailStep bfm_steam_flail_step(BfmSteamKnightFlail *flail,
                                       const BfmSteamKnightStreetRoute *route);
#endif
