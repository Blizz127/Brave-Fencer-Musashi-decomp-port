#include "scene_countdown.h"

s32 func_8017CE40(SceneCountdown *state)
{
    func_8017CFF4();
    state->timer = 10;
    ++state->phase;
    return 0;
}
