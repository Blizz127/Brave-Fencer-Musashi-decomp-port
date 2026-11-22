#include "actor_position_updates.h"
#include "mine_guest_abi.h"

extern void func_8012A988(Obj8014A2E4 *actor);

#if BFM_MINE_DIRECT_GUEST_ADDRESS
#define animation_flags(actor) (*(u16 *)((u8 *)(actor) + 0x72))
#else
static u16 animation_flags(const Obj8014A2E4 *actor)
{
    const u8 *field = (const u8 *)actor + 0x72;
    return (u16)(field[0] | (u16)field[1] << 8);
}
#endif

void func_8012A908(void)
{
    s32 index;
    Obj8014A2E4 *actor = D_801202A0;

    for (index = 0; index < 96; index++, actor++) {
        if (D_801270C0 == 1 || (animation_flags(actor) & 0x1000)) {
            func_8012A988(actor);
        }
    }
}
