#include "steam_knight_core.h"

extern u8 D_801E44E0[], D_801963D0[];
extern u32 D_801D6D94;
extern void func_801858C4(void);
extern void func_8012AD44(u8 *actor, u16 state);
extern void func_80132288(void *effect, const void *record, u32 handle);

#if defined(__mips__) && !defined(__LP64__)
#define CORE_BODY(address) ((SteamKnightPart *)(address))
#else
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
#define CORE_BODY(address) bfm_steam_knight_break_actor(address)
#endif

void func_80184358(SteamKnightPart *core)
{
    core->flags &= ~1;
    if (CORE_BODY(core->body_address)->phase_flags & 2) {
        core->reaction = 1;
        core->timer = 8;
        func_80185A04(core, CORE_BODY(core->body_address), core->damage);
        func_801858C4();
    } else {
        core->reaction = 0;
        core->field_ae = -1;
        func_8012AD44((u8 *)core, 3);
        func_80132288(D_801E44E0, D_801963D0, D_801D6D94);
        func_801858C4();
        func_8002D4C8(1266, 0);
    }
}
