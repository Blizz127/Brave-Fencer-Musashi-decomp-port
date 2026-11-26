/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_CAST_H
#define BFM_STEAM_KNIGHT_CAST_H

#include "forest_terrain.h"

typedef struct BfmSteamKnightHostages BfmSteamKnightHostages;

enum BfmSteamKnightPerformer { BFM_STEAM_ROOTRICK, BFM_STEAM_FILLET };

/* The throne-room actors occupy a separate disc member immediately after
 * the mounted field. They share the field's textures and lighting. */
BfmSteamKnightHostages *bfm_steam_knight_hostages_load(const MusashiDiscMedia *disc, size_t scene_bytes);
void bfm_steam_knight_hostages_free(BfmSteamKnightHostages *cast);

/* Sample a performer-local mounted cycle by its zero-based clip index.
 * The caller owns the world transform, destination pose and draw submission.
 * Clips may be sampled independently for multiple shots. */
int bfm_steam_knight_hostage_actor(const BfmSteamKnightHostages *cast,
                                  enum BfmSteamKnightPerformer who, unsigned clip,
                                  unsigned elapsed, const BfmForestTransform *root,
                                  BfmForestPose *pose, BfmForestActor *out);

/* Select the mounted closed-eye face for this sampled Fillet draw. */
int bfm_steam_knight_fillet_close_eyes(const BfmSteamKnightHostages *cast, BfmForestActor *actor);

#endif
