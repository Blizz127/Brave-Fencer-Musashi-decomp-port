#include "forest_player_state.h"

extern s32 func_8016F0E4(u8 *player);
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
/* The original call forwards the actor in a0, which the callee ignores. */
extern s32 func_80165770(u8 *player);
#else
extern s32 func_80165770(void);
#endif

void func_8017EA2C(ForestPlayerState *player)
{
    func_8016F0E4((u8 *)player);
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    func_80165770((u8 *)player);
#else
    func_80165770();
#endif
}
