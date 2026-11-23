#include "steamwood_pressure.h"
#include "steamwood_valve.h"

void func_8017C628(struct PressureState *state)
{
    func_8002D4C8(27, 0);
    func_80147060(state);
    func_8017BEE8();
    func_80171A04((struct ValvePlayer *)state);
}
