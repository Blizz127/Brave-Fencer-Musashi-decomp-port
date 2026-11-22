#include "mine_frame_callback.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
extern u32 D_8017F850;

void func_8013BCDC(void)
{
    s32 reserved[4];
    if (D_8017F850)
        ((void (*)())D_8017F850)();
}
#else
int bfm_mine_frame_pending_step(void)
{
    u32 target;
    if (!bfm_mine_frame_callback_word(BFM_MINE_PENDING_CALLBACK, &target))
        return 0;
    if (!target)
        return 1;
    if (!bfm_mine_frame_callback_word(BFM_MINE_PENDING_CALLBACK, &target) ||
            !target || (target & 3u))
        return 0;
    return bfm_mine_pending_callback(target);
}

void func_8013BCDC(void)
{
    (void)bfm_mine_frame_pending_step();
}
#endif
