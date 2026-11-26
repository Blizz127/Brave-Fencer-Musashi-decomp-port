#include "forest_player_locomotion.h"

extern void func_80147CC8(void *, s32, s32, s32, void *);
extern void func_80147B5C(u8 *, s32 *);

void func_80147AD4(ForestPlayerState *actor, s32 x, s32 y, s32 z)
{
    s32 position[3];

    func_80147CC8(actor, x, y, z, position);
    func_80147B5C((u8 *)actor, position);
}
