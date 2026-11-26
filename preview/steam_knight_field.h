/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_FIELD_H
#define BFM_STEAM_KNIGHT_FIELD_H
#include "forest_dust.h"
#include "room_descriptor.h"

typedef struct BfmSteamKnightField BfmSteamKnightField;
BfmSteamKnightField *bfm_steam_knight_field_open(const MusashiDiscMedia *disc);
void bfm_steam_knight_field_free(BfmSteamKnightField *field);
/* The scene, terrain, entrance and uploaded tiles belong to the field. */
const uint8_t *bfm_steam_knight_field_scene(const BfmSteamKnightField *field, size_t *size);
BfmForestTerrain *bfm_steam_knight_field_terrain(const BfmSteamKnightField *field);
const RoomTransferDescriptor *bfm_steam_knight_field_entrance(const BfmSteamKnightField *field);
void bfm_steam_knight_entrance_camera(BfmForestCamera *camera);
void bfm_steam_knight_hostage_camera(BfmForestCamera *camera);
void bfm_steam_knight_charge_camera(BfmForestCamera *camera);
void bfm_steam_knight_captor_close_camera(BfmForestCamera *camera);
void bfm_steam_knight_fire_reverse_camera(BfmForestCamera *camera);
void bfm_steam_knight_princess_close_camera(BfmForestCamera *camera);
void bfm_steam_knight_postfire_camera(BfmForestCamera *camera);
/* Omit both hero arguments for a shot that hides Musashi. */
int bfm_steam_knight_field_draw(BfmSteamKnightField *field, const BfmForestCamera *camera,
    const BfmForestPose *pose, const BfmForestTransform *root, uint32_t visible_parts,
    const BfmForestBlade *blade, const BfmForestBladeFrame *blade_frame, unsigned buffer,
    const BfmForestActor *actors, size_t actor_count,
    const BfmForestDustCard *cards, size_t card_count, uint32_t pixels[240][320]);
#endif
