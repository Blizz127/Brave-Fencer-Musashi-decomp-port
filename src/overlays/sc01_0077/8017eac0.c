#include "forest_settle_effect.h"

extern s32 func_8005C49C(void);
extern void func_8017EB2C(s32 random, s32 remaining);
extern s32 func_8012BEE8(u8 *effect);
extern void func_8012C218(u8 *effect);

void func_8017EAC0(ForestSettleEffect *effect)
{
    s32 i;
    for (i = 0; i < 4; ++i) {
        s32 random = func_8005C49C();
        func_8017EB2C(random, effect->remaining);
    }
    if (func_8012BEE8((u8 *)effect) == 1) {
        func_8012C218((u8 *)effect);
    }
}
