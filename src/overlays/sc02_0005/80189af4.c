/* Run the current state of a Steam Knight joint-effect actor. */
#include "steam_knight_hit.h"

#if defined(__mips__) && !defined(__LP64__)
extern void (*D_8019660C[])(SteamKnightPart *effect);
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
extern void bfm_tower_spawn_setup(u32 target, void *actor);
#endif

void func_80189AF4(SteamKnightPart *effect)
{
#if defined(__mips__) && !defined(__LP64__)
    D_8019660C[effect->state](effect);
#else
    u32 target;
    memcpy(&target, bfm_tower_spawn_pointer(0x8019660Cu + 4u * effect->state), sizeof target);
    bfm_tower_spawn_setup(target, effect);
#endif
}
