/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_HURT_H
#define BFM_FOREST_HURT_H
#include "forest_manual.h"

enum { BFM_FOREST_HURT_KEYS = 7 };

typedef struct {
    uint32_t pose[2][BFM_FOREST_HURT_KEYS];
    uint32_t visible;
} BfmForestHurtClip;

typedef struct {
    unsigned active, side, next_key;
} BfmForestHurt;

typedef enum {
    BFM_FOREST_HURT_INVALID = -1, BFM_FOREST_HURT_FREE,
    BFM_FOREST_HURT_LOCKED, BFM_FOREST_HURT_RECOVERED
} BfmForestHurtResult;

int bfm_forest_hurt_load(const MusashiDiscMedia *disc, BfmForestHurtClip *out);
/* Latch a nonlethal ground contact after the player's completed update.
 * Headings use the field model's 4096-unit turn. */
int bfm_forest_hurt_begin(BfmForestHurt *hurt, int body_yaw, int hit_yaw);
/* Step before ground, jump or attack input. LOCKED supplies the logical pose
 * and retains position and heading. RECOVERED consumes the update in standing
 * entry. Read ordinary input on the next update. A room script that owns the
 * actor can pass NULL for both actor and manual to advance only the clock. */
BfmForestHurtResult bfm_forest_hurt_step(BfmForestHurt *hurt, const BfmForestHurtClip *clip,
    ForestPlayerState *actor, BfmForestManual *manual, uint32_t *pose);

typedef struct {
    BfmForestTransform root;
    uint32_t pose, visible;
} BfmForestBodySample;

typedef struct {
    BfmForestBodySample history[2];
} BfmForestHurtDisplay;

/* Feed one logical body sample per update, including motion before contact.
 * The photographed body uses the pose, equipped parts and entire world basis
 * from two updates earlier. Physics uses the current sample. */
BfmForestBodySample bfm_forest_hurt_display(BfmForestHurtDisplay *display,
                                             BfmForestBodySample current);
#endif
