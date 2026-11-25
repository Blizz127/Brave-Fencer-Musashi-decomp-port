/* Owner re-derivation from retail assembly (clean input set). */
#include "scene_command.h"

extern u32 D_800AE6B0;

void func_800D1724(const u8 *descriptor)
{
    D_800AE6B0 = (u32)(SceneAddressInteger)descriptor;
}
