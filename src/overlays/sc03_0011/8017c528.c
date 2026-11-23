#include "steamwood_pressure.h"

void func_8017C528(struct PressureState *state)
{
    if (state->needle >= 68 && state->needle <= 80) {
        func_8017C6F4(state, state->selector);
        func_801542A4(state, D_800D5E18);
        D_80127058 = state->selector + 1;
        func_80147324(1348);
    } else {
        state->valve -= 16;
        if (state->valve < 0)
            state->valve = 0;
    }
    func_80171A80(state, 16);
    func_80171990(state);
}
