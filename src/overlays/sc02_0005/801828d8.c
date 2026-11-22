/* Commit the new body state and refresh the transition bookkeeping. */
#include "steam_knight_hit.h"

extern void func_8012AD44(u8 *body, u16 state);
extern void func_80186F14(SteamKnightPart *body);

void func_801828D8(SteamKnightPart *body)
{
    body->phase_flags |= 1;
    func_8012AD44((u8 *)body, 2);
    func_80186F14(body);
}
