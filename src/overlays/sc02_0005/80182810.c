/* Direction and motion setup for a Steam Knight body transition. */
#include "steam_knight_hit.h"

extern u16 D_801E4BBC;
extern s16 func_80186144(SteamKnightPart *body, int joint);
extern void func_80186304(SteamKnightPart *body, int motion, int mode);

#if defined(__mips__) && !defined(__LP64__)
#define TRANSITION_MODE D_801E4BBC
#define TRANSITION_FLAGS(body) ((body)->phase_flags)
#else
/* Read the mode before a possibly overlapping host-mapped flag store. */
#define TRANSITION_MODE (*(const volatile u16 *)&D_801E4BBC)
#define TRANSITION_FLAGS(body) (*(volatile u32 *)&(body)->phase_flags)
#endif

void func_80182810(SteamKnightPart *body)
{
    u32 flags;
    int mode;

    if (body->phase_flags & 0x800) {
        if (body->phase_flags & 0x2000)
            body->motion_x = -0x38000;
        else
            body->motion_x = -0x1c000;
        body->motion_z = 0;
        body->field_e0 = func_80186144(body, 5);
    } else {
        if (body->phase_flags & 0x2000)
            body->motion_z = 0x38000;
        else
            body->motion_z = 0x1c000;
        body->motion_x = 0;
        body->field_e0 = -1;
    }
    flags = body->phase_flags;
    mode = TRANSITION_MODE;
    TRANSITION_FLAGS(body) = flags | 0x1000;
    if (mode == 1)
        func_80186304(body, -3, 0);
    else
        func_80186304(body, 3, 0);
}
