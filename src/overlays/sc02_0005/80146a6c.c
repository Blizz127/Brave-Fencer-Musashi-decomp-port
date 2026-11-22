#include "steam_knight_effect.h"

SteamKnightEffect *func_80146A6C(int kind, u32 owner_address, int x, int y,
                                 s16 z, int mode, int amount)
{
    SteamKnightEffectRequest request;

    request.kind = kind;
    request.owner_address = owner_address;
    request.x = x;
    request.y = y;
    request.z = z;
    request.mode = mode;
    request.amount = amount;
    return func_80146AFC(&request);
}
