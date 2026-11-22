/* Clamp the actor field and run the four state services in their original order. */
#include "../../../include/forest_player_state.h"

extern u32 D_8011F730;
extern void func_80161124(ForestPlayerState *actor);
extern void func_801477E8(ForestPlayerState *actor, s32 amount);
extern s32 func_8014C010(s32 actor_address, s32 mode);
extern s32 func_80146CA0(ForestPlayerState *actor);

s32 func_8015ADB0(ForestPlayerState *actor)
{
    if (actor->field_a8 > 24)
        actor->field_a8 = 24;
    func_80161124(actor);
    func_801477E8(actor, -0x120000);
    func_8014C010(FOREST_ACTOR_GAME_ADDRESS(actor), 2);
    D_8011F730 |= 1;
    return func_80146CA0(actor);
}
