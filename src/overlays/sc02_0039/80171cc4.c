/* Check the planar target after moving, even when the first move is complete. */
#include "mine_award_early.h"

extern s32 func_80171D78(u8 *actor, u8 *vector);
extern s32 func_8017250C(u8 *actor, u8 *vector);
extern void func_80146D90(u8 *actor);

s32 func_80171CC4(u8 *actor, u8 *vector)
{
    s32 reached = func_80171D78(actor, vector);
    if (func_8017250C(actor, vector))
        func_80146D90(actor);
    return reached;
}
