#include "mine_fade_lifecycle.h"

void func_8013A4C4(ScreenWork *work)
{
    ScreenEffect *effect;
    u32 address;
#if !BFM_SCREEN_DIRECT
    if (!work)
        return;
#endif
    address = fade_address(work);
    set_fade_half(work->target, 0);
    effect = fade_resolve(address);
#if !BFM_SCREEN_DIRECT
    if (!effect)
        return;
#endif
    set_fade_half(effect->factors.x, fade_half(effect->factors.x) - 1024);
    set_fade_half(effect->factors.y, fade_half(effect->factors.y) - 1024);
    if (fade_signed(effect->factors.x) <= 1024) {
        set_fade_half(effect->factors.y, 0);
        set_fade_half(effect->factors.x, 0);
        set_state(work, 4);
        set_substate(work, 0);
        forget_effect(work);
    } else {
        func_8013A530(work);
    }
}

#if !BFM_SCREEN_DIRECT
void bfm_mine_fade_finish(ScreenWork *work)
{
    func_8013A4C4(work);
}
#endif
