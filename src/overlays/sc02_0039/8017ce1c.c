#include "scene_countdown.h"

extern s32 func_800167F0(u16 phase);

s32 func_8017CE1C(SceneCountdown *state)
{
    (void)state;
    return (u16)func_800167F0(0) != 0;
}
