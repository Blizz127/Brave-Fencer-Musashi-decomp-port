#include "steam_knight_core.h"

extern u16 D_801E4BBC;
extern s16 D_80195B24[];

s16 func_80185A88(const SteamKnightPart *body, s16 incoming)
{
    s16 current = (u16)body->hp - (u16)body->damage;
    s16 after = current - incoming;
    s16 limited = after;
    s32 floor;

    if (D_801E4BBC == 2) {
        if (!(body->phase_flags & 0x800) && after < 128)
            limited = 128;
    } else if (!(body->phase_flags & 0x80000)) {
        floor = D_80195B24[D_801E4BBC] >> 1;
        if (after < floor)
            limited = floor;
    }
    return current - limited;
}
