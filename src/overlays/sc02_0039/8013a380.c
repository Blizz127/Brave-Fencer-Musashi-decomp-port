#include "mine_actor_fade.h"

extern ScreenWork D_8011DA80[];

#if BFM_MINE_DIRECT_GUEST_ADDRESS
extern void func_8013A448(ScreenWork *slot);
extern void func_8013A4C4(ScreenWork *slot);
#define slot_state(slot) (*(s16 *)((slot)->reserved00))
#define fade_start func_8013A448
#define fade_finish func_8013A4C4
#else
#define slot_state(slot) ((slot)->reserved00[0] | (unsigned)(slot)->reserved00[1] << 8)
#define fade_start bfm_mine_fade_start
#define fade_finish bfm_mine_fade_finish
#endif

void func_8013A380(void)
{
    ScreenWork *slot = D_8011DA80;
    s32 i;
    for (i = 0; i < 2; ++i, ++slot) {
        if (slot) {
            switch (slot_state(slot)) {
            case 2:
                func_8013A530(slot);
                break;
            case 1:
                fade_start(slot);
                break;
            case 3:
                fade_finish(slot);
                break;
            }
        }
    }
}
