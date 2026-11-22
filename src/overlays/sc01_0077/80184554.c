/* Start the second state and its twenty-field countdown. */
#include "psx_types.h"
extern s32 func_8012A828(u8 *actor, u32 animation);
extern u8 D_801973C4[];
void func_80184554(u8 *actor)
{
    *(u16 *)(actor + 2) = 2;
    func_8012A828(actor, (u32)(unsigned long)D_801973C4);
    *(s32 *)(actor + 0x1c) = 20;
}
