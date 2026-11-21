#include "opening_event.h"

extern u8 D_800D5A34[];
extern s32 func_800CF854(void);
extern u32 func_8013767C(u32 address);
extern void func_80154274(u8 *player, u32 record);
extern s32 func_8014706C(u8 *player);

#if HISTORICAL_OPENING_ABI
void func_8017DF5C(ScenePlayer *player)
{
#else
void func_8017DF5C(u8 *object)
{
    ScenePlayer *player = (ScenePlayer *)(void *)object;
#endif
    if (func_80171B4C(player, 30)) {
        if (func_800CF854()) {
            player->event_state = 4;
        } else {
            player->effect_address = func_8013767C(player->effect_address);
            func_80154274((u8 *)player, RECORD_ADDRESS(800D5A34));
            func_8014706C((u8 *)player);
            player->event_progress.phase = 0;
            ++player->event_state;
        }
    }
}
