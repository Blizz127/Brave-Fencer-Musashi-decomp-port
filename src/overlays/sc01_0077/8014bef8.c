/* The player request uses the low byte of game flag 128's result. */
#include "../../../include/forest_action_event.h"

s32 func_8014BEF8(ForestPlayerState *actor)
{
    (void)actor;
    return (u8)func_80029178(0x80);
}
