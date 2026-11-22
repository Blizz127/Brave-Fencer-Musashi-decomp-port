/* Submit the actor's event request when either player predicate accepts it. */
#include "../../../include/forest_action_event.h"

s32 func_801498E0(u32 actor_address)
{
    ForestActionEventRecord request;

    if (func_80149AA8(FOREST_EVENT_ACTOR(actor_address)) ||
        func_80149B54(FOREST_EVENT_ACTOR(actor_address))) {
        request.type = 8;
        request.actor = actor_address;
        request.zero = 0;
        request.amount = -30;
        request.phase = 0;
        request.first = 0;
        request.second = 0;
        func_80146750(&request);
        return 1;
    }
    return 0;
}
