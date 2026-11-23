/* Owner re-derivation from retail assembly (clean input set). */
#include "game_option_values.h"

extern u8 func_800291B4(s32 index);

s32 func_80028FBC(void)
{
    return func_800291B4(44) & 0xff;
}
