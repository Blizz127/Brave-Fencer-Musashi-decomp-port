#include "scene_countdown.h"

extern ScenePhaseHandler D_80182B4C[];

s32 func_8017CF1C(SceneCountdown *state)
{
    return D_80182B4C[state->phase](state);
}
