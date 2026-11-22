#include "mine_door_action.h"

void func_80150B28(MineDoorActionPlayer *player) {
    if ((D_800AE6B0 & 0x80ffffffu) == 0) {
        u16 action = player->action;
        if (action != 0 && action < 38) {
            u32 index = action;
            --index;
            D_80180D78[index](player);
        }
    }
    player->action = 0;
}
