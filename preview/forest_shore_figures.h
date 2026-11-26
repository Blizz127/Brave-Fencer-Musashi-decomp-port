/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHORE_FIGURES_H
#define BFM_FOREST_SHORE_FIGURES_H
#include "forest_field.h"

typedef struct BfmForestShoreFigures BfmForestShoreFigures;

enum { BFM_FOREST_SHORE_FIGURES = 4 };

/* The four creek placements each have a moving body and a rigid base.
 * The mounted field must outlive the figures. */
BfmForestShoreFigures *bfm_forest_shore_figures_open(const BfmForestField *field);
void bfm_forest_shore_figures_free(BfmForestShoreFigures *figures);

/* Called after the player's position has been committed for this field update. */
void bfm_forest_shore_figures_tick(BfmForestShoreFigures *figures,
                                   const int16_t position[3]);

/* During a player injury the aim cache can hold its earlier location. The
 * actor still measures field admission and retirement from the current root. */
void bfm_forest_shore_figures_tick_aim(BfmForestShoreFigures *figures, const int16_t player[3],
                                      const int16_t aim[3], int targetable);

typedef struct {
    int16_t position[3], yaw;
    unsigned behavior, tick;
    int16_t health;
    uint16_t flags, tag;
    int32_t position_q16[3];
} BfmForestShoreFigureState;

/* Report a registered creature's last completed field update. */
int bfm_forest_shore_figure_state(const BfmForestShoreFigures *figures, unsigned placement,
                                  BfmForestShoreFigureState *out);

/* The caller visits active targets in field registration order. A successful
 * ray returns 1 for an accepted ordinary strike or 4 for a blocking surface.
 * A miss, unavailable target or previously queued type preserves all outputs. */
int bfm_forest_shore_figure_contact(BfmForestShoreFigures *figures, unsigned placement,
                                   const int16_t start[3], const int16_t end[3],
                                   uint16_t kind, uint16_t strength, int16_t attacker_yaw,
                                   int16_t center[3], int16_t point[3], int16_t normal[3]);

/* The player visits registered bodies in slot order. Probing is read-only:
 * a facing check and the shorter pickup reach can query the same creature. */
int bfm_forest_shore_figure_probe(const BfmForestShoreFigures *figures, unsigned placement,
                                 uint16_t flag_mask, const int16_t start[3], const int16_t end[3],
                                 int16_t contact[3]);
int bfm_forest_shore_figures_block_rock(const BfmForestShoreFigures *figures,
                                     const int16_t from[3], const int16_t to[3]);
int bfm_forest_shore_figure_grip_offset(const BfmForestShoreFigures *figures, int16_t out[3]);
int bfm_forest_shore_figure_grab(BfmForestShoreFigures *figures, unsigned placement,
                                uint16_t power, int16_t player_yaw);
/* The player supplies the whole world coordinates from its mounted hold track.
 * The creature retains its position fractions between pickup and release. */
int bfm_forest_shore_figure_hold(BfmForestShoreFigures *figures, unsigned placement,
                                const int16_t world[3], int16_t player_yaw);
/* Queue a forward throw or an upright drop when a carry is interrupted. */
int bfm_forest_shore_figure_throw(BfmForestShoreFigures *figures, unsigned placement,
                                 uint16_t power, int16_t player_yaw);

int bfm_forest_shore_figure_drop(BfmForestShoreFigures *figures, unsigned placement,
                                uint16_t power, int16_t player_yaw);

/* Return complete parent/base pairs for the room's shared depth pass.
 * If the destination is too small, leave it untouched. */
size_t bfm_forest_shore_figures_actors(const BfmForestShoreFigures *figures,
                                      BfmForestActor *out, size_t capacity);
#endif
