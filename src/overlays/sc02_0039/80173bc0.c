/* Resolve the scripted selector before advancing or switching to phase seven. */
#include "mine_award_early.h"

void func_80173BC0(u8 *actor)
{
    if (D_80126B3A != 0 && D_80126B3A < 111 && !func_800D0EC4()) {
        if ((s16)D_80126B38 & 0x8000) {
            func_8013E588();
        } else {
            func_80174650(actor);
            D_8011F9C8 = 2;
            func_80171A34(actor, 7);
            return;
        }
    }
    func_80171A1C(actor);
}
