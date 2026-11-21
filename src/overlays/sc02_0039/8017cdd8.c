#include "scene_countdown.h"

extern void func_800167B8(s32 phase);

s32 func_8017CDD8(SceneCountdown *state)
{
    func_8017CFCC();
    func_800167B8(0);
    ++state->phase;
    return 0;
}
