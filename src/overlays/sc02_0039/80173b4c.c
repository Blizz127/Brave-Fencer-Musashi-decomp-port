/* Invoke the player's current award phase through the loaded room table. */
#include "mine_award_controller.h"

void func_80173B4C(u8 *actor)
{
    D_8018271C[actor[0x214]](actor);
}
