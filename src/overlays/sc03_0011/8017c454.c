#include "steamwood_pressure.h"

void func_8017C454(struct PressureState *state)
{
    func_8002D4C8(26, 0);
    state->action_state = 0;
    func_8014706C(state);
    func_80154274(state, D_800D5DF4);
    state->valve = 0;
    state->needle = 0;
    func_8017BEBC(&state->valve, &state->needle);
    func_80171990(state);
    func_8017C4C4(state);
}
