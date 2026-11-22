#include "steam_knight_core.h"


void func_80185A04(SteamKnightPart *core, SteamKnightPart *body, s16 amount)
{
    s16 accepted;
    if (!(body->phase_flags & 4)) {
        body->phase_flags |= 0x10;
        accepted = func_80185A88(body, amount);
        body->damage += accepted;
        func_8016AA50(core, accepted);
#if defined(__mips__) && !defined(__LP64__)
        *(u16 *)((u8 *)body + 0xDE) = 8;
#else
        ((u8 *)body)[0xDE] = 8;
        ((u8 *)body)[0xDF] = 0;
#endif
        func_801869E0(body);
    }
}
