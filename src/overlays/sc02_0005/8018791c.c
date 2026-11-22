/* Advance the Steam Knight's body while it is in state two. */
#include "steam_knight_hit.h"

extern void func_80182998(SteamKnightPart *body);
extern void func_80182918(SteamKnightPart *body);
extern int func_8018687C(SteamKnightPart *body);
extern int func_8018675C(SteamKnightPart *body);
extern int func_8012BEE8(SteamKnightPart *body);
extern void func_801838A8(SteamKnightPart *body);

void func_8018791C(SteamKnightPart *body)
{
    if (body->phase_flags & 0x60000) {
        func_80182998(body);
    } else {
        func_80182918(body);
        if (!(body->phase_flags & 0x1000) && body->break_delay > 0)
            --body->break_delay;
        if (func_8018687C(body) || func_8018675C(body) || func_8012BEE8(body)) {
            body->phase_flags &= ~0x1000u;
            func_801838A8(body);
        }
    }
}
