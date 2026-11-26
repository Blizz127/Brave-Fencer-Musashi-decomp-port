/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_HERO_MOTION_H
#define BFM_STEAM_KNIGHT_HERO_MOTION_H

#include "forest_field.h"
#include "forest_manual.h"
#include "forest_lift.h"
#include "forest_swing.h"
#include "forest_cast.h"
#include "steam_knight_rotation.h"
#include "steam_knight_corner.h"

typedef struct BfmSteamKnightHeroMotion BfmSteamKnightHeroMotion;
typedef struct {
    int issued, roll_strength;
    unsigned power_class;
    uint16_t type;
    size_t count;
    int16_t from[3], ends[BFM_FOREST_STRIKE_RAYS][3];
} BfmSteamKnightHeroStrike;

typedef struct {
    int16_t feet[3], heading;
    unsigned action, motion, elapsed;
    uint32_t body;
} BfmSteamKnightHeroSample;

/* The handover supplies the actual script feet, facing, and the same walking
 * startup speed as the existing player. This controller owns no RPG state. */
BfmSteamKnightHeroMotion *bfm_steam_hero_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const int16_t feet[3], int16_t heading,
    int32_t initial_walk_velocity);
/* A scripted gate supplies its actual feet and heading after the held target
 * has left the hand. Refuse unsupported positions and preserve the last pose
 * on failure. A successful placement starts idle with no fractional movement. */
int bfm_steam_hero_place(BfmSteamKnightHeroMotion *hero, const int16_t feet[3], int16_t heading);
void bfm_steam_hero_free(BfmSteamKnightHeroMotion *hero);
const BfmSteamKnightHeroSample *bfm_steam_hero_sample(const BfmSteamKnightHeroMotion *hero);
uint32_t bfm_steam_hero_flags(const BfmSteamKnightHeroMotion *hero);
const BfmForestField *bfm_steam_hero_surface(const BfmSteamKnightHeroMotion *hero);
BfmSteamCornerStep bfm_steam_hero_corner(BfmSteamKnightHeroMotion *hero,
                                         BfmSteamCorner *corner, int view_ready);
/* One ordinary field update. x and y are the live controller axes (right/down
 * positive). Segments are valid only for this update's active blade window. */
int bfm_steam_hero_tick(BfmSteamKnightHeroMotion *hero, const BfmForestCamera *camera,
    int x, int y, uint16_t pressed, uint16_t held, BfmSteamKnightHeroStrike *strike);
/* The target owner first checks its live lift permission and the world grab
 * segment, then accepts the target on the same Square edge before hero_tick.
 * Mounted room data and grip are borrowed through the end of the throw. */
int bfm_steam_hero_lift_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                            size_t bytes, BfmForestLiftData *out);
/* The room supplies a confirmed body sweep after the ordinary ground tick.
 * The previous contact supplies the next input's brace eligibility. Omit the
 * call, or pass NULL feet, when a normal ground step no longer contacts it. */
int bfm_steam_hero_brace(BfmSteamKnightHeroMotion *hero, const int16_t corrected_feet[3],
                         const BfmForestLiftData *data);
int bfm_steam_hero_grab_segment(const BfmSteamKnightHeroMotion *hero, uint16_t pressed,
                                int16_t from[3], int16_t to[3]);
int bfm_steam_hero_grab(BfmSteamKnightHeroMotion *hero, uint16_t pressed,
                        const BfmForestLiftData *data, const int16_t local_grip[3]);
/* ATTACH and WALK_ATTACH publish the held actor origin. RELEASE is the single
 * mounted release marker. Other steps leave the output coordinates untouched. */
BfmLiftStep bfm_steam_hero_carry(const BfmSteamKnightHeroMotion *hero, int16_t world[3]);
typedef enum {
    BFM_STEAM_LUMINA_ERROR = -1, BFM_STEAM_LUMINA_PASS, BFM_STEAM_LUMINA_STANCE,
    BFM_STEAM_LUMINA_ROTATE, BFM_STEAM_LUMINA_FINISHED, BFM_STEAM_LUMINA_SLASH
} BfmSteamHeroLumina;
/* Run before the ordinary hero tick. PASS delegates to that tick. SLASH
 * requests the distinct ordinary Triangle action. STANCE, ROTATE and FINISHED
 * have advanced the hero. Each active ray retains its own start and end. */
BfmSteamHeroLumina bfm_steam_hero_lumina(BfmSteamKnightHeroMotion *hero,
    const BfmLuminaRotation *rotation, BfmForestStatus *player,
    const BfmForestCamera *camera, int x, int y, uint16_t pressed, uint16_t held,
    BfmSteamFlameRay rays[BFM_ROTATION_RAYS], size_t *count);
/* An independently established physical impact may interrupt the current
 * action. Heading is the world-space direction of knockback. The caller owns HP. */
int bfm_steam_hero_recoil(BfmSteamKnightHeroMotion *hero, int16_t heading);

#endif
