#include "steamwood_pressure.h"

void func_8017C4C4(struct PressureState *state)
{
    if (func_80148800(state) & 0x10) {
        state->phase = 5;
    } else {
        func_8017C6B8(state, state->selector);
        if (func_80148800(state) & 0x40)
            func_80171990(state);
    }
}
