#include "steamwood_pressure.h"

void func_8017C6B8(struct PressureState *state, unsigned int selector)
{
    state->needle += D_80184D98[selector];
    if (state->needle > 96)
        state->needle = 0;
}
