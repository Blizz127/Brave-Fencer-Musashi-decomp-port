/* Arm the configured collision flags and start the third state's animation and timer. */
#include "psx_types.h"
extern s32 func_8012A828(u8 *actor, u32 animation);
extern u8 D_80196F3C[];
void func_80184594(u8 *actor)
{
    u8 *configuration;
    *(u16 *)(actor + 2) = 3;
    func_8012A828(actor, (u32)(unsigned long)D_80196F3C);
    configuration = (u8 *)(unsigned long)*(u32 *)(actor + 0x78);
    *(u16 *)(actor + 0x5c) = *(u16 *)(configuration + 2);
    *(u16 *)(actor + 0x5e) = 0;
    *(u16 *)(actor + 0xdc) = 0;
    *(s32 *)(actor + 0x1c) = 45;
}
