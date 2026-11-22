/* Find the first idle slot in the forward action range. */
#include "steam_knight_action_search.h"

s32 func_8018AC48(SteamKnightPart *body)
{
    int index;
    SteamKnightPart *part;

    for (index = 0; index < 12; index++) {
        part = ACTION_SLOT(index);
        if (part && !SLOT_BUSY(part) &&
            (u16)(SLOT_Z(body) - SLOT_Z(part) - 193) < 255)
            return index;
    }
    return -1;
}
