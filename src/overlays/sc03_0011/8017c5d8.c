#include "steamwood_pressure.h"

void func_8017C5D8(struct PressureState *state)
{
    if (func_80171A88(state)) {
        if (state->valve == 96) {
            state->phase = 4;
        } else {
            state->needle = 0;
            state->phase = 1;
        }
    }
}
