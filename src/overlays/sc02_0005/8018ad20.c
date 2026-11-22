/* Find an idle even-numbered slot beside the Steam Knight. */
#include "steam_knight_action_search.h"

s32 func_8018AD20(SteamKnightPart *body)
{
    int pair;
    int index;
    SteamKnightPart *part;

    for (pair = 0; pair < 6; pair++) {
        index = pair * 2;
        part = ACTION_SLOT(index);
        if (part && !SLOT_BUSY(part) &&
            (u16)(SLOT_Z(body) - SLOT_Z(part) + 3) < 7)
            return index;
    }
    return -1;
}
