/* Restore the prior foot position and its status as the rescue target. */
#include "psx_types.h"

typedef struct {
    s16 position[3];
    s16 status;
} TargetPoint;

typedef struct {
    u8 before_prior_foot[0x90];
    TargetPoint prior_foot;
    u8 before_target[0x15c - 0x98];
    TargetPoint target;
} RescuePlayer;

void func_8014C8F0(RescuePlayer *player)
{
    player->target = player->prior_foot;
}
