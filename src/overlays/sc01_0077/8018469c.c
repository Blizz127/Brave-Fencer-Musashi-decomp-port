/* Begin the close action and request its one-time auxiliary actor. */
#include "psx_types.h"

extern s32 func_8012A828(u8 *actor,u32 animation);
extern void func_80142414(s32 owner,s32 offset);
extern u8 D_801970DC[];

void func_8018469C(u8 *actor)
{
    *(u16 *)(actor + 2) = 4;
    func_8012A828(actor, (u32)(unsigned long)D_801970DC);
    *(s32 *)(actor + 0x1c) = 30;
    if (!*(s16 *)(actor + 0xfc)) {
        *(u16 *)(actor + 0xfc) = 1;
        func_80142414((s32)(unsigned long)actor, -150);
    }
}
