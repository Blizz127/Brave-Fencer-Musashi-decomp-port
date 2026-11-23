#include "steamwood_pressure.h"

void func_8017C404(struct PressureState *state)
{
    D_80183A10[state->phase](state);
    func_80148634(state);
}
