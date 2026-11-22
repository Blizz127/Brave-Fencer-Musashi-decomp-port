/* Once its timer expires the actor may select its next action. Otherwise it
 * retains the room's normal retirement test. */
#include "psx_types.h"

extern s32 func_8012BEE8(u8 *actor);
extern s32 func_801845EC(u8 *actor,s32 near_limit,s32 far_limit);
extern s32 func_8012C044(u8 *actor);
extern void func_8012C098(u8 *actor);

void func_80184B24(u8 *actor)
{
    if (func_8012BEE8(actor) == 1) {
        if ((*(s16 *)(actor + 0x70) & 0x8000) ?
            func_801845EC(actor, 0x40000, 0xc4000) == 1 :
            func_801845EC(actor, 0x9000, 0x24000) == 1) {
            return;
        }
    }
    if (func_8012C044(actor)) {
        func_8012C098(actor);
    }
}
