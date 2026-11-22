/* A nonzero state selects its one-based handler in the loaded mine member. */
#include "mine_frame_state.h"

void func_801627E8(void)
{
    MineFrameState *state = &D_8011F750;
    s32 index;

    if (state->state != 0) {
        index = state->state - 1;
        D_80181CA0[index](state);
    }
}
