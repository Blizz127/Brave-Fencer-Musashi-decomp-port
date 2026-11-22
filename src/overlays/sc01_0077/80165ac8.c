#include "forest_player_state.h"

extern u8 D_80078EC1;
#if defined(__mips__) && !defined(__LP64__)
extern u32 D_80189974[];
#endif

void func_80165AC8(char *player)
{
    FOREST_PLAYER_CALL(80189974, D_80078EC1 & 0x7f, (ForestPlayerState *)player);
}
