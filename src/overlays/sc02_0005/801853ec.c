#include "steam_knight_hit.h"

void func_801853EC(SteamKnightPart *part)
{
    part->hp -= part->damage;
    part->flags &= ~1;
    func_8016AA50(part, part->damage);

    if (part->hp <= 0) {
        part->hit_kind = 0;
        func_80185060(part, 1);
        func_80186B08(part);
    } else {
        part->reaction = 1;
        part->timer = 8;
        func_80186A04(part);
    }
    if (part->hit_kind != 29) {
        func_8002D4C8(1267, 0);
    }
}
