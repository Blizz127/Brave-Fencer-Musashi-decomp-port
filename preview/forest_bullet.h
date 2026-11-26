/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BULLET_H
#define BFM_FOREST_BULLET_H
#include "forest_pose.h"

/* The field reserves projectile slots 8 through 19 for special attacks. */
enum { BFM_FOREST_BULLETS = 12 };
typedef struct {
    uint32_t position[3]; /* wrapping 16.16 room coordinates */
    int32_t velocity[3];
    unsigned remaining, active;
} BfmForestBullet;
typedef struct { BfmForestBullet shots[BFM_FOREST_BULLETS]; } BfmForestBullets;

typedef int (*BfmGunshotContact)(void *context, const int16_t from[3],
                                 const int16_t to[3], int16_t hit[3], int16_t normal[3]);
typedef enum {
    BFM_BULLET_FADE = 3, BFM_BULLET_SURFACE_HIT = 7, BFM_BULLET_ACTOR_HIT = 31
} BfmBulletEffect;
typedef struct {
    BfmBulletEffect kind;
    int16_t position[3], normal[3];
    unsigned slot;
} BfmBulletEvent;

/* Fire on the earned skill's release marker with the current hero model.
 * Pitch and roll are auxiliary angles; yaw is the model heading. The projectile
 * is invisible. The firing action submits its own muzzle flash. */
int bfm_forest_bullet_launch(BfmForestBullets *pool, const BfmForestTransform *root,
                             int16_t pitch, int16_t yaw, int16_t roll);

/* Tick existing shots before accepting new releases. Positive contact means
 * a struck actor, negative a solid surface. A hit supplies its intersection and outward normal.
 * The optional event array accommodates one terminal effect per active shot.
 * A change of equipped skill removes the projectile. */
size_t bfm_forest_bullets_tick(BfmForestBullets *pool, unsigned ability,
                               BfmGunshotContact contact, void *context,
                               BfmBulletEvent events[BFM_FOREST_BULLETS]);
#endif
