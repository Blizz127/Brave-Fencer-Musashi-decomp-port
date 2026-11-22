/* Configure the forest actor and its child attachment, then start its idle animation. */
#include "psx_types.h"

typedef struct { u8 configuration[52]; } Type12Config;
extern Type12Config D_8018AEB8[];
extern u8 D_8018AF20[], D_8019731C[];
extern s32 func_8012C354(u8 *actor, Type12Config *config);
extern s32 func_8012C658(s16 type, s16 selector, u8 *owner);
extern s32 func_8012CAE4(u8 *actor);
extern s32 func_8012A828(u8 *actor, u32 animation);

void func_80184C0C(u8 *actor)
{
    s32 child;
    if (func_8012C354(actor, &D_8018AEB8[*(u16 *)(actor + 0x70) & 1])) {
        actor[0xc0] = 1;
        *(s32 *)(actor + 0xb4) = -1;
        *(u32 *)(actor + 0xbc) = (u32)(unsigned long)D_8018AF20;
        *(s32 *)(actor + 0xc4) = 1;
        child = func_8012C658(121, *(u16 *)(actor + 0x70) & 1, actor);
        *(s32 *)(actor + 0x6c) = child;
        if (!child) {
            func_8012CAE4(actor);
        } else {
            *(u16 *)(actor + 2) = 1;
            func_8012A828(actor, (u32)(unsigned long)D_8019731C);
        }
    }
}
