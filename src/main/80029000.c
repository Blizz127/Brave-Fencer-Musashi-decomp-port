/* Owner re-derivation from retail assembly (clean input set). */
#include "game_option_values.h"

extern u8 func_800291B4(s32 index);

s32 func_80029000(void)
{
    return func_800291B4(45) & 0xff;
}
