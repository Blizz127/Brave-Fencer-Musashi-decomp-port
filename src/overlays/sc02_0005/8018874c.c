#include "steam_knight_hit.h"

extern s32 D_801E43A0;
extern void func_80185214(SteamKnightPart *part);

#if defined(__mips__) && !defined(__LP64__)
#define LINK(address) ((SteamKnightPart *)(address))
#else
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
#define LINK(address) bfm_steam_knight_break_actor(address)
#endif

#if defined(__mips__) && !defined(__LP64__) && defined(__GNUC__) && __GNUC__ == 2
#define SHIFT(variant) ((variant) + 1)
#else
#define SHIFT(variant) (((u32)(variant) + 1u) & 31u)
#endif

void func_8018874C(SteamKnightPart *part)
{
    s16 variant = part->variant;
    if (variant < 13 && ((D_801E43A0 >> SHIFT(variant)) & 1))
        func_80185060(part, 0);
    if (LINK(part->linked_address)->phase_flags & 0x8000)
        func_80185214(part);
}
