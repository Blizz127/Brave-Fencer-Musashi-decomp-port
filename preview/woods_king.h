/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_KING_H
#define BFM_WOODS_KING_H

#include "bedroom_figure.h"

typedef struct BfmWoodsKing BfmWoodsKing;
typedef struct {
    unsigned sound, bite_power, body_points;
    int body_reward;
} BfmWoodsKingEvent;

/* The scene stays mounted while the King and its morphs are in the field. */
BfmWoodsKing *bfm_woods_king_open(const uint8_t *scene, size_t bytes, const uint8_t flags[64]);
void bfm_woods_king_free(BfmWoodsKing *king);
int bfm_woods_king_add(const BfmWoodsKing *king, BfmBedroomFigure *figure,
                       const BfmBedroomView *view);

/* Update the actor before field movement. A geometrically connected bite
 * supplies power to the player's ordinary defense and hurt handling. */
int bfm_woods_king_step(BfmWoodsKing *king, const int16_t previous_player[3],
                        uint8_t flags[64], BfmWoodsKingEvent *event);
/* Room movement makes two collision passes. field_tick selects the common
 * alternating inside-push offset; both passes use the same update counter. */
int bfm_woods_king_slide(const BfmWoodsKing *king, const int16_t previous[3],
                         int16_t endpoint[3], unsigned field_tick);
/* Check the corrected feet after movement, before the spell advances. */
void bfm_woods_king_touch(BfmWoodsKing *king, const int16_t player[3]);

/* Apply a connected ordinary sword attack. K. Maneater plays its planted
 * recoil; this reaction awards no damage number or defeat reward. */
int bfm_woods_king_strike(BfmWoodsKing *king, unsigned cause,
                          const int16_t from[3], const int16_t to[3]);

/* Shrink uses the spell spine's distance to the actor root. Its caller
 * dispatches a hit only when the eligible root is within the spell radius. */
const int16_t *bfm_woods_king_shrink_root(const BfmWoodsKing *king);
int bfm_woods_king_shrink(BfmWoodsKing *king);

#endif
