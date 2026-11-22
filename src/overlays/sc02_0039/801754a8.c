/* Publish the mode flag before calling the mine member's current handler. */
#include "mine_frame_state.h"

void func_801754A8(void)
{
    MineFrameMode *state = &D_8011F7A8;
    u32 tick = func_80029504();
    u8 flag = 0;

    if ((D_800B9A13 == 0 && (D_80126B9C & 0x80) == 0) || tick - 10 >= 0x4B0) {
        flag = 1;
    }
    state->flag = flag;
    D_801828E8[state->mode](state);
}
