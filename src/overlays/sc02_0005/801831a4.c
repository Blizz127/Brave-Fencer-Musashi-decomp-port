#include "steam_knight_hit.h"

extern void func_80186600(SteamKnightPart *body);
extern void func_80183218(SteamKnightPart *body);
extern void func_80186DE8(SteamKnightPart *body);
#if defined(__mips__) && !defined(__LP64__)
extern void (*D_801962C0[])(SteamKnightPart *);
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
extern void bfm_tower_spawn_setup(u32 target, void *actor);
#endif

void func_801831A4(SteamKnightPart *body)
{
    func_80186600(body);
    func_80183218(body);
#if defined(__mips__) && !defined(__LP64__)
    D_801962C0[body->state](body);
#else
    {
        u32 target;
        memcpy(&target, bfm_tower_spawn_pointer(0x801962C0u + 4u * body->state),
               sizeof target);
        bfm_tower_spawn_setup(target, body);
    }
#endif
    func_80186DE8(body);
}
