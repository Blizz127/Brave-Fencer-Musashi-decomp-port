#include "steam_knight_hit.h"

extern void func_801870BC(SteamKnightPart *part);
extern void func_80185214(SteamKnightPart *part);

#if defined(__mips__) && !defined(__LP64__)
#define LINK(address) ((SteamKnightPart *)(address))
#else
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
#define LINK(address) bfm_steam_knight_break_actor(address)
#endif

void func_801887CC(SteamKnightPart *part)
{
    if (part->break_delay != 0) {
        --part->break_delay;
        func_801870BC(part);
    }
    if (LINK(part->linked_address)->phase_flags & 0x8000)
        func_80185214(part);
}
