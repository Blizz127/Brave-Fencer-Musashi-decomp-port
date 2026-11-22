/* The queued worker supplies a sixth word after the eight-byte vector. */
#include "mine_award_controller.h"

void func_80173A28(s32 setup, s32 selector, s32 type, s32 id, const MineAwardVector *vector, u32 next)
{
    func_80173A60(setup, selector, type, id, vector);
    D_8012750C = next;
}
