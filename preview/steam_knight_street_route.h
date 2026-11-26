/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_STREET_ROUTE_H
#define BFM_STEAM_KNIGHT_STREET_ROUTE_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    int32_t root_q12[3];
    unsigned age, ticks;
    int west;
    int32_t step_q12;
} BfmSteamKnightStreetWalk;
typedef enum {
    BFM_STREET_WALK_INVALID, BFM_STREET_WALK_MOVING, BFM_STREET_WALK_EXPIRED,
    BFM_STREET_WALK_CROSSED_DEPTH, BFM_STREET_WALK_CROSSED_WEST
} BfmSteamKnightStreetStep;

enum { BFM_STREET_ROWS = 32, BFM_STREET_MARKERS = 4 };
typedef struct {
    int16_t x, z, phase;
} BfmSteamStreetMarker;
typedef struct {
    struct { int16_t delay; uint16_t action; } rows[BFM_STREET_ROWS];
    unsigned length, cursor;
    uint16_t pending_walks;
    int16_t delay, depth_limit, west_limit, marker_tags[BFM_STREET_MARKERS];
    BfmSteamStreetMarker markers[BFM_STREET_MARKERS];
} BfmSteamKnightStreetRoute;
typedef enum {
    BFM_STREET_IDLE, BFM_STREET_FORWARD, BFM_STREET_LEFT, BFM_STREET_SHOT,
    BFM_STREET_MARKED_SHOT, BFM_STREET_JUMP, BFM_STREET_MARKED_JUMP,
    BFM_STREET_CHOICE_SEVEN
} BfmSteamStreetAction;
typedef struct {
    BfmSteamStreetAction action;
    int marker; /* -1 when this specific action has no selected marker. */
} BfmSteamStreetChoice;

/* The live encounter owns phase, HP, target impacts and the current marker. */
int bfm_steam_street_route_load(BfmSteamKnightStreetRoute *route,
                                const uint8_t *scene, size_t bytes);
int bfm_steam_street_route_choose(BfmSteamKnightStreetRoute *route, uint32_t flags,
                                  int hp, int current_marker, BfmSteamStreetChoice *out);
void bfm_steam_street_delay_tick(BfmSteamKnightStreetRoute *route);
void bfm_steam_street_depth_crossed(BfmSteamKnightStreetRoute *route, uint32_t *flags);
/* Pick a destination at the top of this jump, using the player's current
 * position. The last unmarked jump returns to the start of the whole action. */
int bfm_steam_street_jump_target(const BfmSteamKnightStreetRoute *route, int marker,
    unsigned jumps_left, uint32_t flags, const int16_t action_start[3],
    const int16_t body[3], const int16_t hero[3], int16_t xz[2]);
/* During a westward stride or roll the chassis can cross the selected
 * house row. Zero means the street phase stays unchanged. */
int bfm_steam_street_roll_phase(const BfmSteamKnightStreetRoute *route, int marker,
                               uint32_t flags, int16_t x);
/* Age zero displays the first pose at the inherited fractional actor root. */
int bfm_steam_street_walk_begin(BfmSteamKnightStreetWalk *walk, const int32_t root_q12[3],
                                uint32_t body_flags);
/* On crossing the depth or west boundary the caller updates the route flags,
 * retires queued strides, and runs the ordinary action chooser. */
BfmSteamKnightStreetStep bfm_steam_street_walk_tick(BfmSteamKnightStreetWalk *walk,
                                                    int16_t boundary);
#endif
