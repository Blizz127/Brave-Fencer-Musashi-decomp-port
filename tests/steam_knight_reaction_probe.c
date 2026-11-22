#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include "steam_knight_hit.h"

typedef struct { s16 x, y, z, id; } SteamKnightReaction;
s32 D_801E43A0;
u16 D_80185B4C[32768 + 14];
SteamKnightReaction D_801961B8[4], D_801961E0[4];

static void run(int variant, u32 mask, u16 lookup, int second,
                const s16 before[4], const s16 expected[4])
{
    SteamKnightPart actor;
    SteamKnightReaction fixed[4], alternate[4];
    SteamKnightReaction *selected = second ? D_801961E0 : D_801961B8;
    SteamKnightReaction *other = second ? D_801961B8 : D_801961E0;
    unsigned i;

    memset(&actor, 0x59, sizeof actor);
    actor.variant = (s16)variant;
    SteamKnightPart saved = actor;
    D_801E43A0 = (s32)mask;
    if (variant < 14) D_80185B4C[32768 + variant] = lookup;

    for (i = 0; i < 4; ++i) {
        selected[i] = (SteamKnightReaction){(s16)(80+i), (s16)(300-i), (s16)(-123+i), before[i]};
        other[i] = (SteamKnightReaction){(s16)(-765+i), (s16)(-982-i), (s16)(335+i), (s16)(40+i)};
    }
    memcpy(fixed, selected, sizeof fixed);
    memcpy(alternate, other, sizeof alternate);
    func_80186A04(&actor);
    for (i = 0; i < 4; ++i) {
        fixed[i].id = expected[i];
    }
    assert(!memcmp(fixed, selected, sizeof fixed));
    assert(!memcmp(alternate, other, sizeof alternate));
    assert(!memcmp(&actor, &saved, sizeof actor));
    assert((u32)D_801E43A0 == mask);
}

#define IDS(a,b,c,d) ((const s16[4]){(a),(b),(c),(d)})

int main(void)
{
    _Static_assert(offsetof(SteamKnightPart, variant) == 0x70, "variant offset");
    _Static_assert(sizeof(SteamKnightReaction) == 8, "record size");
    run(0, 0, 5, 0, IDS(1,4,7,-1), IDS(1,4,5,7));
    run(9, 0, 2, 0, IDS(3,4,6,7), IDS(2,3,4,6));
    run(13, 0, 20, 0, IDS(1,4,6,7), IDS(1,4,6,7));
    run(8, 0, 18, 0, IDS(1,18,19,-1), IDS(1,18,19,-1));
    run(6, 0, 14, 0, IDS(1,-1,19,20), IDS(1,14,19,20));
    run(5, 0, 22, 1, IDS(-1,9,10,12), IDS(0,9,10,12));
    run(3, 0, 30, 1, IDS(1,3,7,12), IDS(1,3,7,8));
    run(7, 0, 29, 1, IDS(-3,3,-1,12), IDS(-3,3,7,12));
    run(-1, 1, 4, 0, IDS(1,3,5,-1), IDS(1,3,5,-1));
    run(-2, 0x80000000u, 23, 1, IDS(0,-1,-1,-1), IDS(0,-1,-1,-1));
    run(-33, 1, 4, 0, IDS(1,3,5,-1), IDS(1,3,5,-1));
    run(SHRT_MIN, 0, 15, 0, IDS(2,9,-1,-1), IDS(2,9,15,-1));
    run(4, 1u<<4, 8, 0, IDS(3,5,9,-1), IDS(3,5,8,9));
    run(4, 1u<<5, 8, 0, IDS(3,5,9,-1), IDS(3,5,9,-1));
    run(14, 0, 0, 0, IDS(-1,-1,-1,-1), IDS(-1,-1,-1,-1));
    run(SHRT_MAX, 0, 32, 1, IDS(-1,-1,-1,-1), IDS(-1,-1,-1,-1));
    return 0;
}
