#include "../../../include/forest_transition.h"

s32 func_8015C0C4(ForestActionPlayer *actor)
{
    /* The corresponding signed word type also preserves the original compiler schedule. */
    *(s32 *)&actor->step = (s32)(s16)D_801152B0.x * 64;
    *(s32 *)&actor->counter = 0x80000;
    *(s32 *)&actor->field_23c = (s32)FOREST_NORMAL_Z * 64;
    func_80154150(actor, 17);
    func_801553A8(actor);
    return func_80146CA0(actor);
}
