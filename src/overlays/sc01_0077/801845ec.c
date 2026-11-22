/* While the timer has expired, choose a close attack or return to idle after
 * the actor has waited long enough for the player to leave its outer range. */
#include "psx_types.h"

extern s32 func_8012BCCC(u8 *actor);
extern void func_8018469C(u8 *actor);
extern s32 func_8012A828(u8 *actor,u32 animation);
extern u8 D_8019731C[];

s32 func_801845EC(u8 *actor,s32 near_limit,s32 far_limit)
{
    s32 distance = func_8012BCCC(actor);
    if (distance <= near_limit) {
        func_8018469C(actor);
        return 1;
    }
    if (++*(s16 *)(actor + 0xdc) > 120) {
        if (distance > far_limit) {
            *(u16 *)(actor + 2) = 1;
            func_8012A828(actor, (u32)(unsigned long)D_8019731C);
            return 1;
        }
        return 0;
    }
    return 0;
}
