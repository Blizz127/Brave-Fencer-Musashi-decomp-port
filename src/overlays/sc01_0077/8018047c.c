#include "psx_types.h"

extern u16 D_80126B96;
extern s32 func_8012BEE8(u8 *actor);
extern s32 func_8012D714(u8 *actor, s32 mode);
extern void func_8002D4C8(u16 command, u16 argument);
extern void func_8017F694(u8 *actor);

void func_8018047C(u8 *actor)
{
    u32 speed;
    u8 *model;

    if (actor[0x70] == 4 || func_8012BEE8(actor)) {
        speed = *(u32 *)(actor + 0xe0) + *(u32 *)(actor + 0xe4);
        model = (u8 *)(unsigned long)*(u32 *)(actor + 0x20);
        *(u32 *)(actor + 0xdc) += speed;
        *(u32 *)(actor + 0xe0) = speed;
        *(u16 *)(model + 0x10) = *(u16 *)(actor + 0xde);
        if (*(s16 *)(actor + 0xde) >= 0x400) {
            if (*(u16 *)(actor + 0x34) == 0 && func_8012D714(actor, 2))
                D_80126B96 = 0x4002;
            if (*(u16 *)(actor + 0x34) >= 2) {
                func_8017F694(actor);
            } else {
                if (*(u16 *)(actor + 0x34) == 0)
                    func_8002D4C8(0x487, 0);
                *(u32 *)(actor + 0xdc) = 0x04000000u;
                *(u16 *)(actor + 0x5c) |= 0xcc00;
                *(s32 *)(actor + 0xe0) = -(*(s32 *)(actor + 0xe0) >> 1);
                ++*(u16 *)(actor + 0x34);
            }
        }
    }
}
