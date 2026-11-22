#include "forest_settle_effect.h"

extern void func_8012AD44(u8 *effect, u16 state);

void func_8017EA98(ForestSettleEffect *effect)
{
    effect->remaining = 16;
    func_8012AD44((u8 *)effect, 1);
}
