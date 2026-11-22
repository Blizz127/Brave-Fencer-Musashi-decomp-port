/* Turn the model toward its target, then change action when the timer expires. */
#include "psx_types.h"

extern s32 func_8012B8E4(u8 *actor,s16 rate);
extern s32 func_8012BEE8(u8 *actor);
extern void func_801846F8(u8 *actor);

void func_80184BB4(u8 *actor)
{
    s32 angle = func_8012B8E4(actor, 4);
    u8 *model = (u8 *)(unsigned long)*(u32 *)(actor + 0x20);
    *(u16 *)(model + 0x12) += angle;
    if (func_8012BEE8(actor)) {
        func_801846F8(actor);
    }
}
