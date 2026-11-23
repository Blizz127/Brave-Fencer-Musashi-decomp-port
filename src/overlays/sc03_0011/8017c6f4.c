#include "steamwood_pressure.h"

void func_8017C6F4(struct PressureState *state, unsigned int selector)
{
    state->valve += D_80184DA0[selector];
    if (state->valve > 96)
        state->valve = 96;
}
