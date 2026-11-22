#include "steam_knight_hit.h"

void func_80183200(SteamKnightPart *body)
{
    body->reaction = 0;
    body->hit_kind = 0;
    body->flags &= ~1u;
}
