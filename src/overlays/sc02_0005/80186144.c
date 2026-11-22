/* Return the current marker index when its action matches. */
#include "steam_knight_action_search.h"

#if defined(__mips__) && !defined(__LP64__)
extern s16 D_801E43B4;
extern s16 D_80195AF8[][4];
#define MARKER D_801E43B4
#define TAG(index) D_80195AF8[index][0]
#else
#define MARKER ((s16)bfm_action_half(bfm_tower_spawn_pointer(0x801e43b4u), 0))
#define TAG(index) ((s16)bfm_action_half(bfm_tower_spawn_pointer(0x80195af8u + 8u * (u32)(index)), 0))
#endif

s16 func_80186144(SteamKnightPart *body, int action)
{
    s16 index = MARKER;
    (void)body;
    if (TAG(index) == action)
        return index;
    return -1;
}
