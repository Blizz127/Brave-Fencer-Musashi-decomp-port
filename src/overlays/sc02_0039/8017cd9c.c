#include "scene_countdown.h"

extern ScenePhaseHandler D_80182B38[];

s32 func_8017CD9C(SceneCountdown *state)
{
    return D_80182B38[state->phase](state);
}
