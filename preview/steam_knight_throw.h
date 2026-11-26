/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_THROW_H
#define BFM_STEAM_KNIGHT_THROW_H
#include <stdint.h>

enum BfmSteamThrowEvent {
    BFM_STEAM_THROW_LIFTABLE = 1u << 0, BFM_STEAM_THROW_GATE_CROSSED = 1u << 1,
    BFM_STEAM_THROW_HERO_REPOSITION = 1u << 2, BFM_STEAM_THROW_BODY_ARRIVED = 1u << 3,
    BFM_STEAM_THROW_WEST_FALL = 1u << 4, BFM_STEAM_THROW_WEST_REPLAY = 1u << 5,
    BFM_STEAM_THROW_WEST_IMPACT = 1u << 6
};

typedef struct {
    int16_t body[3], apex[3];
    int32_t position[3], velocity[3], floor_y, contact_y;
    unsigned round, phase, age, released_age, crossed;
} BfmSteamKnightThrow;

/* Zero-initialize. Start at the real zero-HP knockdown. The next step moves it. */
int bfm_steam_throw_collapse(BfmSteamKnightThrow *m, unsigned round, const int16_t body[3]);
/* Preserve fractional world coordinates when the live actor supplies them. */
int bfm_steam_throw_collapse_q12(BfmSteamKnightThrow *m, unsigned round, const int32_t body[3]);
/* Call at the held action's release marker after an accepted lift. */
int bfm_steam_throw_release(BfmSteamKnightThrow *m, const int16_t held_body[3]);
/* One world update. Read body after stepping. Events are issued once. */
unsigned bfm_steam_throw_step(BfmSteamKnightThrow *m);
#endif
