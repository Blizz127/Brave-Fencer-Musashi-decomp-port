#include "forest_player_state.h"
#include "forest_actor_contact.h"

extern s32 D_801270D4;
extern u32 func_80172658(u8 *player);
extern void func_80171B44(u8 *player);

void func_8017E8A4(ForestPlayerState *player)
{
    u16 action = player->kind;
    if (action != 0x19 && action != 0x1a) {
        if (D_801270D4 >= 30) {
            player->scene_stage = 4;
        } else if (func_80172658((u8 *)player)) {
            u32 model = player->model;
            *FOREST_ACTOR_REF(s16, model + 0x12) = 0;
            func_80171B44((u8 *)player);
            ++player->scene_stage;
        }
    }
}
