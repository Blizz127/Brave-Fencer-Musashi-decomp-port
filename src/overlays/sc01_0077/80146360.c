/* Select the active forest player state from the player's state byte. */
#include "../../../include/forest_player_state.h"

void func_80146360(void)
{
    FOREST_PLAYER_CALL(8018860C, FOREST_CURRENT_STATE, &D_80126B58);
}
