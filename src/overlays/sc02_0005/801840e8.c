#include "steam_knight_hit.h"

#if defined(__mips__) && !defined(__LP64__)
#define PARENT(address) ((SteamKnightPart *)(address))
#else
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
#define PARENT(address) bfm_steam_knight_break_actor(address)
#endif

void func_801840E8(SteamKnightPart *core)
{
    if (PARENT(core->body_address)->phase_flags & 2) {
        if (core->field_0e == 24) {
            core->field_fc |= 1;
            func_8002D4C8(1290, 0);
        }
    } else {
        core->field_fc |= 1;
    }
}
