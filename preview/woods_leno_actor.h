/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LENO_ACTOR_H
#define BFM_WOODS_LENO_ACTOR_H
#include "bedroom_figure.h"

typedef struct BfmWoodsLenoActor BfmWoodsLenoActor;

typedef enum {
    BFM_LENO_REST, BFM_LENO_BARK, BFM_LENO_APPROACH, BFM_LENO_CHEW,
    BFM_LENO_HELD, BFM_LENO_TOSSED, BFM_LENO_LANDED, BFM_LENO_CLIPS
} BfmWoodsLenoClip;

typedef struct {
    int16_t xyz[3];
    int yaw, pitch;
    BfmWoodsLenoClip clip;
    unsigned tick;       /* Effective animation updates since the current clip began. */
    int visible;         /* A blinking dog can retain its separate floor shadow. */
} BfmWoodsLenoView;

/* Keep the mounted 3025 field scene alive while this actor is in use. */
BfmWoodsLenoActor *bfm_woods_leno_actor_open(const uint8_t *scene, size_t bytes);
void bfm_woods_leno_actor_free(BfmWoodsLenoActor *actor);
int bfm_woods_leno_actor_home(const BfmWoodsLenoActor *actor, int16_t dog[3],
                              int16_t bone[3], int *yaw);
unsigned bfm_woods_leno_actor_duration(const BfmWoodsLenoActor *actor, BfmWoodsLenoClip clip);
/* Append at field depths after preparing Musashi. NULL dog submits the bone alone. */
int bfm_woods_leno_actor_add(const BfmWoodsLenoActor *actor, BfmBedroomFigure *figure,
                             const BfmBedroomView *view, const BfmWoodsLenoView *dog);
#endif
