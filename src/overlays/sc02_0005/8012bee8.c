/* Tick a Tower actor's duration, treating its stored value as a wrapping word. */
#include "steam_knight_hit.h"

#if !(defined(__mips__) && !defined(__LP64__))
#include <string.h>
#endif

s32 func_8012BEE8(SteamKnightPart *body)
{
    u32 remaining = (u32)body->timer;

    if (remaining != 0) {
        remaining--;
#if defined(__mips__) && !defined(__LP64__)
        body->timer = (s32)remaining;
#else
        memcpy(&body->timer, &remaining, sizeof remaining);
#endif
        return remaining == 0;
    }
    return 1;
}
