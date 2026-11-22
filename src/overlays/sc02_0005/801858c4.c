#include "steam_knight_hit.h"
extern u32 D_801E43C0, D_801E43C4;
extern s32 func_8012C218(u8 *actor);

#if defined(__mips__) && !defined(__LP64__)
#define TRACKED_ACTOR(address) ((u8 *)(address))
#else
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
#define TRACKED_ACTOR(address) ((u8 *)bfm_steam_knight_break_actor(address))
#endif

void func_801858C4(void)
{
    u32 *tracked = &D_801E43C0;
    if (*tracked) {
        func_8012C218(TRACKED_ACTOR(*tracked));
        *tracked = 0;
    }
    if (D_801E43C4) {
        func_8012C218(TRACKED_ACTOR(D_801E43C4));
        D_801E43C4 = 0;
    }
}
