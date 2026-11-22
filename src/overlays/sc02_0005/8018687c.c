/* Clamp the moving body's X coordinate to the current round's walls. */
#include "steam_knight_limits.h"

s32 func_8018687C(SteamKnightPart *body)
{
#if defined(__mips__) && !defined(__LP64__)
    /* The original leaf reserves 16 unreferenced stack bytes. */
    u8 frame[16];
#endif
    s16 wall;
    s32 upper;
    s32 lower;
#if defined(__mips__) && !defined(__LP64__)
    (void)frame;
#endif

    if (!(body->phase_flags & 0x800)) {
        s32 motion = body->motion_x;
        if (motion > 0) {
            wall = upper = X_UPPER(ROUND_INDEX);
            if (upper < POS_X(body))
                goto clamp;
            motion = ((volatile SteamKnightPart *)body)->motion_x;
        }
        if (motion < 0) {
            wall = lower = X_LOWER(ROUND_INDEX);
            if (POS_X(body) < lower) {
clamp:
                SET_X(body, wall);
                return 1;
            }
        }
    }
    return 0;
}
