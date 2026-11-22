#include "steam_knight_hit.h"
#include "steam_knight_reaction.h"

void func_80186B08(SteamKnightPart *part)
{
    int variant = part->variant;
    u16 id;
    SteamKnightReaction *list;
    int i, j;

    if (variant >= 14) return;
    id = D_80185B4C[32768 + variant];
    if (id >= 22) {
        list = D_801961E0;
        id -= 22;
    } else {
        list = D_801961B8;
    }
    for (i = 0; i < 4; ++i) {
        if (list[i].id == id) {
            for (j = i; j < 4; ++j) list[j].id = list[j + 1].id;
            return;
        }
    }
}
