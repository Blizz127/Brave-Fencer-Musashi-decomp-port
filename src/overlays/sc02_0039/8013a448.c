#include "mine_fade_lifecycle.h"

void func_8013A448(ScreenWork *work)
{
    ScreenEffect *effect;
    u16 phase;
#if !BFM_SCREEN_DIRECT
    if (!work)
        return;
#endif
    phase = fade_substate(work);
    effect = fade_resolve(fade_address(work));
#if !BFM_SCREEN_DIRECT
    if (!effect)
        return;
#endif
    if (!phase) {
        set_fade_half(effect->factors.y, 1024);
        set_fade_half(effect->factors.x, 1024);
        set_substate(work, fade_substate(work) + 1);
    } else {
        set_fade_half(effect->factors.x, fade_half(effect->factors.x) + 512);
        set_fade_half(effect->factors.y, fade_half(effect->factors.y) + 512);
        if (fade_signed(effect->factors.x) == 4096) {
            set_state(work, 2);
            set_substate(work, 0);
        }
    }
    func_8013A530(work);
}

#if !BFM_SCREEN_DIRECT
void bfm_mine_fade_start(ScreenWork *work)
{
    func_8013A448(work);
}
#endif
