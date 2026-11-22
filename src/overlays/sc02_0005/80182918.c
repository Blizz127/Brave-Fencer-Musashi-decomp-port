/* Update the Steam Knight body and play the transition cues on frame one. */
#include "steam_knight_hit.h"

extern s32 func_8012CBA4(SteamKnightPart *body);
extern void func_8018655C(SteamKnightPart *body);

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_80191060[];
extern void func_8013C9C4(const void *descriptor);
#define TRANSITION_SCRIPT() func_8013C9C4(D_80191060)
#define TRANSITION_MOTION(body) (*(s32 *)((u8 *)(body) + 0x94))
#define TRANSITION_FRAME(body) (*(s16 *)((u8 *)(body) + 0x98))
#else
#include <string.h>
/* The room service receives a serialized pointer into the active overlay. */
extern void bfm_steam_knight_transition_script(u32 descriptor_address);
#define TRANSITION_SCRIPT() bfm_steam_knight_transition_script(0x80191060u)

static s32 transition_motion(const SteamKnightPart *body)
{
    s32 value;
    memcpy(&value, (const u8 *)body + 0x94, sizeof(value));
    return value;
}

static s16 transition_frame(const SteamKnightPart *body)
{
    s16 value;
    memcpy(&value, (const u8 *)body + 0x98, sizeof(value));
    return value;
}

#define TRANSITION_MOTION(body) transition_motion(body)
#define TRANSITION_FRAME(body) transition_frame(body)
#endif

void func_80182918(SteamKnightPart *body)
{
    s32 motion;

    func_8012CBA4(body);
    motion = TRANSITION_MOTION(body);
    if ((motion == 1 || motion == 13) && TRANSITION_FRAME(body) == 1) {
        TRANSITION_SCRIPT();
        func_8002D4C8(1265, 0);
        func_8002D4C8(1291, 0);
    }
    func_8018655C(body);
}
