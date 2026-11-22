/* Update the first active state of the Steam Knight's body. */
#include "steam_knight_hit.h"

int func_8012CE2C(SteamKnightPart *body);
void func_8012CBCC(SteamKnightPart *body);
void func_80182998(SteamKnightPart *body);
void func_8018655C(SteamKnightPart *body);
int func_8012BEE8(SteamKnightPart *body);
void func_801838A8(SteamKnightPart *body);

#if defined(__mips__) && !defined(__LP64__)
#define BODY_ANIMATION(body) (*(const u32 *)((const u8 *)(body) + 0x94))
#define BODY_FRAME(body) (*(const s16 *)((const u8 *)(body) + 0x98))
#else
#include <string.h>

static u32 body_animation(const SteamKnightPart *body)
{
    u32 animation;
    memcpy(&animation, (const u8 *)body + 0x94, sizeof animation);
    return animation;
}

static s16 body_frame(const SteamKnightPart *body)
{
    s16 frame;
    memcpy(&frame, (const u8 *)body + 0x98, sizeof frame);
    return frame;
}
#define BODY_ANIMATION(body) body_animation(body)
#define BODY_FRAME(body) body_frame(body)
#endif

void func_801877FC(SteamKnightPart *body)
{
    u32 animation;
    if (!func_8012CE2C(body))
        func_8012CBCC(body);
    if (body->phase_flags & 0x60000u) {
        func_80182998(body);
        return;
    }
    func_8018655C(body);
    if (body->break_delay > 0)
        body->break_delay--;
    if (func_8012BEE8(body))
        func_801838A8(body);
    animation = BODY_ANIMATION(body);
    if ((animation == 4 || animation == 12) && BODY_FRAME(body) == 1)
        func_8002D4C8(0x4f7, 0);
}
