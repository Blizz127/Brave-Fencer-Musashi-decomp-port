#include "steamwood_timer.h"

void func_8017BEE8(void)
{
    struct ValveEffect *effect;

    func_80147364(4, 0x542);
    func_80147364(4, 0x543);
    func_80147364(4, 0x541);
    effect = func_8014C050(&D_80126B58, 41);
    if (effect) {
        if (effect->primary)
            func_80016714(effect->primary, 56);
        if (effect->parts[0])
            func_80016714(effect->parts[0], 56);
        if (effect->parts[1])
            func_80016714(effect->parts[1], 56);
        if (effect->parts[2])
            func_80016714(effect->parts[2], 56);
        if (effect->parts[3])
            func_80016714(effect->parts[3], 56);
        func_80146C3C(effect);
    }
}
