/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_EARTH_RETURN_CAST_H
#define BFM_VILLAGE_EARTH_RETURN_CAST_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmVillageEarthReturnCast BfmVillageEarthReturnCast;
typedef enum {
    BFM_EARTH_CAST_APPROACH,
    BFM_EARTH_CAST_IDLE,
    BFM_EARTH_CAST_FACE_HERO,
    BFM_EARTH_CAST_SPEAK,
    BFM_EARTH_CAST_CALL_DOG,
    BFM_EARTH_CAST_FINISH_LOOK,
    BFM_EARTH_CAST_WALK_POSE,
    BFM_EARTH_CAST_TURN,
    BFM_EARTH_CAST_WALK,
    BFM_EARTH_CAST_HIDE_JON,
    BFM_EARTH_CAST_RELEASE_DOG
} BfmEarthCastCommand;

/* The caller owns the 1011/2 story admission and retains the mounted upper
 * village scene. Opening starts Jon and his child at the actual actor station. */
BfmVillageEarthReturnCast *bfm_village_earth_return_cast_open(const uint8_t *scene, size_t bytes);
void bfm_village_earth_return_cast_free(BfmVillageEarthReturnCast *cast);
/* APPROACH takes packed signed X/Z. TURN takes relative yaw in the low half.
 * WALK takes signed forward speed in Q8 and the field count in the high half.
 * FACE_HERO samples the supplied live player's root at dispatch. */
int bfm_village_earth_return_cast_command(BfmVillageEarthReturnCast *cast, BfmEarthCastCommand command,
                                          uint32_t argument, const int16_t hero[3]);
/* Called once per live update, including the arrival reveal and dialogue.
 * The dog consumes its normal shared random draws while it waits. */
int bfm_village_earth_return_cast_step(BfmVillageEarthReturnCast *cast, BfmGameplayRun *run,
                                       int flag266);
const int16_t *bfm_village_earth_return_cast_position(const BfmVillageEarthReturnCast *cast, int dog);
int bfm_village_earth_return_cast_add(const BfmVillageEarthReturnCast *cast,
                                     BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
