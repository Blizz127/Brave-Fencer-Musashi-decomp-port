#include "../../../include/forest_secondary_event.h"

s32 func_80149954(ForestPlayerState *actor)
{
    u16 position[4];
    ForestActionEventRecord request;

    if (actor->flags & 0x404)
        return 0;
    if (!(actor->field_aa & 0x90)) {
        if (D_800B9A08 == 0x308d) {
            FOREST_SECONDARY_POSITION(position);
            func_8012E5CC(position, 4, 0x8e0);
            return 0;
        }
        FOREST_SECONDARY_SOUND(4, 0x8e0);
        return 0;
    }
    if (actor->field_ac & 0x90) {
        if (D_8012707C == 0x8e0)
            D_8012707C = 0;
        else
            FOREST_SECONDARY_PLAYER_SOUND(0x8e0, 0);
    }
    FOREST_SECONDARY_UPDATE(actor);
    request.type = 0x12;
    request.actor = FOREST_ACTOR_GAME_ADDRESS(actor);
    request.zero = 0;
    request.amount = -30;
    request.phase = 0;
    request.first = 0;
    request.second = 0;
    func_80146750(&request);
    return 1;
}
