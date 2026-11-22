#include "steam_knight_actor84.h"

#if defined(__mips__) && !defined(__LP64__)
extern void (*D_801966B8[])(SteamKnightPart *);
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
extern void bfm_tower_spawn_setup(u32 target, void *actor);
#endif

void func_80189CCC(SteamKnightPart *actor)
{
#if defined(__mips__) && !defined(__LP64__)
    D_801966B8[actor->state](actor);
#else
    u32 target;
    /* The state selects a live 32-bit game callback. */
    memcpy(&target, bfm_tower_spawn_pointer(0x801966B8u + 4u * actor->state),
           sizeof target);
    bfm_tower_spawn_setup(target, actor);
#endif
}
