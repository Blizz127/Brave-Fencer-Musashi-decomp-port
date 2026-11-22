/* Select the actor's bit-seven request only when neither blocking flag is set. */
#include "../../../include/forest_action_event.h"

s32 func_80149AA8(ForestPlayerState *actor)
{
    u32 requested;

    if (!(actor->flags & 0x404)) {
        requested = actor->field_ac & 0x80;
        return requested != 0;
    }
    return 0;
}
