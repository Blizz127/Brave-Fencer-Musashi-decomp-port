#include "scene_countdown.h"

extern void func_8002D4C8(u16 command, u16 argument);
extern void func_8001BFD0(void);
extern void func_800D1E28(void);

s32 func_8017CF58(SceneCountdown *state)
{
    if (--state->timer == (u32)-1) {
        func_8002D4C8(0x1c, 0);
        func_8001BFD0();
        func_8002D4C8(0x1d, 0);
        func_800D1E28();
        ++state->phase;
    }
    return 0;
}
