/* Read the actor bit after the game flag query returns. */
#include "../../../include/forest_action_event.h"

s32 func_80149B54(ForestPlayerState *actor)
{
    if (actor->flags & 0x400)
        return 0;
    if (!func_8014BEF8(actor))
        return 0;
    if (actor->field_ac & 0x10)
        return 1;
    return 0;
}
