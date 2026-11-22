#include "actor_position_updates.h"
#include "mine_guest_abi.h"

extern void func_80178744(u8 *actor, s32 scale);

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
#include <stdint.h>

static int actor_in_pool(const Obj8014A2E4 *actor)
{
    uintptr_t address = (uintptr_t)actor;
    uintptr_t base = (uintptr_t)D_801202A0;
    return address >= base && address - base < 96 * sizeof(*actor)
        && (address - base) % sizeof(*actor) == 0;
}
#endif

void func_8017869C(Obj8014A2E4 *actor)
{
    s32 ticks;
    s32 scale;

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    if (!actor_in_pool(actor)) {
        bfm_actor_update_refused(BFM_ACTOR_UPDATE_BAD_ACTOR, 0, 0xaa);
        return;
    }
#endif
    if (actor->hp == 0) {
        return;
    }
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    ticks = --actor->hp;
#else
    ticks = actor->hp == -32768 ? 32767 : actor->hp - 1;
    actor->hp = (s16)ticks;
#endif
    if (ticks == 0) {
        scale = 0x1000;
#if BFM_MINE_DIRECT_GUEST_ADDRESS
        if (*(u16 *)((u8 *)actor + 0x5e) == 0x1e) {
            *(u16 *)((u8 *)actor + 0x5e) = 0;
        }
#else
        u8 *mode = (u8 *)actor + 0x5e;
        if (mode[0] == 0x1e && mode[1] == 0) {
            mode[0] = 0;
            mode[1] = 0;
        }
#endif
    } else if (ticks > 240) {
        scale = 0x1000 - (256 - ticks) * 0x90;
    } else if (ticks < 16) {
        scale = 0x1000 - ticks * 0x90;
    } else {
        scale = 0x700;
    }
    func_80178744((u8 *)actor, scale);
}
