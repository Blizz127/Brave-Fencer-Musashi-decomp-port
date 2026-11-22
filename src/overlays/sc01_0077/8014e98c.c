/* Apply two actor-contact scans, refreshing the endpoints for the second pass. */
#include "forest_actor_contact.h"

s32 func_8014E98C(u8 *actor)
{
    FloorPoint probe, end;
    s32 result = 0;
    s32 pass = 2;
    if (!(FOREST_ACTOR_WORD(actor + 0x44) & 0x80000) &&
        !(FOREST_ACTOR_WORD(actor + 0x1f8) & 0x4000000)) {
        do {
            probe.x = FOREST_ACTOR_HALF(actor + 0x88);
            probe.y = FOREST_ACTOR_HALF(actor + 0x8a) - 16;
            probe.z = FOREST_ACTOR_HALF(actor + 0x8c);
            end.x = FOREST_ACTOR_HALF(actor + 6);
            end.y = FOREST_ACTOR_HALF(actor + 0xa) - 16;
            end.z = FOREST_ACTOR_HALF(actor + 0xe);
            --pass;
            result |= func_8014EA4C(actor, &probe, &end, pass);
        } while (pass);
    }
    return result;
}
