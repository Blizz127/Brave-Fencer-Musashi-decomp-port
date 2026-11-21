#include "opening_event.h"

extern u8 D_8018A62C[], D_8018A69C[], D_8018A6E4[];
extern s32 func_8016F410(s32 flag_index);
extern void func_80154274(u8 *player, u32 record);
extern s32 func_801399F0(u8 *effect);
extern s32 func_80139914(u8 *effect);
extern s32 func_80147060(u8 *player);

#if HISTORICAL_OPENING_ABI
void func_8017DFD8(ScenePlayer *player)
{
#else
void func_8017DFD8(u8 *object)
{
    ScenePlayer *player = (ScenePlayer *)(void *)object;
#endif
    if (func_8016F410(0)) {
        func_80154274((u8 *)player, RECORD_ADDRESS(8018A62C));
    }
    if (func_8016F410(1)) {
        func_80154274((u8 *)player, RECORD_ADDRESS(8018A69C));
        player->event_progress.phase = 1;
    }
    if (player->event_progress.phase == 1 && (player->animation_status & 0x8000)) {
        player->event_progress.phase = 2;
        func_80154274((u8 *)player, RECORD_ADDRESS(8018A6E4));
    }
    if (func_801399F0(OPENING_OBJECT(u8, player->effect_address))) {
        func_80139914(OPENING_OBJECT(u8, player->effect_address));
        func_80147060((u8 *)player);
        ++player->event_state;
    }
}
