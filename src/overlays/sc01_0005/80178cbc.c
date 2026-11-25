#include "scene_command.h"

extern u32 D_801CD3B8;
extern s16 D_801CE02C;
extern void func_80178D40(void *actor, const u32 *command);

void func_80178CBC(void *actor, const u32 *commands)
{
    D_801CD3B8 = (u32)(SceneAddressInteger)commands;
    do {
        func_80178D40(actor, (const u32 *)(SceneAddressInteger)D_801CD3B8);
    } while (D_801CE02C == 3);
}
