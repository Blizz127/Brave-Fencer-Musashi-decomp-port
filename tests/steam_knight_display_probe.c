#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "steam_knight_hit.h"

_Static_assert(sizeof(SteamKnightPart) == 268, "actor size");
_Static_assert(offsetof(SteamKnightPart, display_record) == 88, "display field");
typedef void (*ChangeDisplay)(SteamKnightPart *, u16);
_Static_assert(_Generic(&func_80187280, ChangeDisplay: 1, default: 0), "display ABI");

u16 D_80185B4C[32768 + 14];
u32 D_801BD9D4[13], D_801E4418[22], D_801E4474[22];
s32 D_801E43A0;

static void check_selection(u16 key, u16 route)
{
    SteamKnightPart part, expected_part;
    u32 expected_banks[2][22];
    for (unsigned i = 0; i < 22; ++i) {
        D_801E4418[i] = 0x28a45013u + i;
        D_801E4474[i] = 0xe36b07b8u - i;
    }
    for (unsigned i = 0; i < 13; ++i)
        D_801BD9D4[i] = 0x3e4c1207u + i;
    D_80185B4C[32767 + key] = route;
    D_801E43A0 = (s32)0xb17f0005u;
    memset(&part, 0x59, sizeof part);

    memcpy(&expected_part, &part, sizeof part);
    expected_part.display_record = (0x80195ca4u + 16u * (key - 1u)) | 0x40000000u;
    memcpy(expected_banks[0], D_801E4418, sizeof D_801E4418);
    memcpy(expected_banks[1], D_801E4474, sizeof D_801E4474);
    unsigned slot = route % 22;
    expected_banks[route / 22][slot] = slot == 5 ? 0x01000000u : D_801BD9D4[key - 1];

    func_80187280(&part, key);
    assert(memcmp(&part, &expected_part, sizeof part) == 0);
    assert(memcmp(expected_banks[0], D_801E4418, sizeof D_801E4418) == 0);
    assert(memcmp(expected_banks[1], D_801E4474, sizeof D_801E4474) == 0);
    assert((u32)D_801E43A0 == (0xb17f0005u | (1u << key)));
}

static void check_without_actor(void)
{
    u32 before[2][22];
    memcpy(before[0], D_801E4418, sizeof D_801E4418);
    memcpy(before[1], D_801E4474, sizeof D_801E4474);
    D_801E43A0 = 0x5a08u;
    func_80187280(NULL, 0);
    assert(D_801E43A0 == 0x5a09u);
    for (unsigned key = 14; key <= 65535; ++key)
        func_80187280(NULL, (u16)key);
    assert(D_801E43A0 == 0x5a09u);
    assert(memcmp(before[0], D_801E4418, sizeof D_801E4418) == 0);
    assert(memcmp(before[1], D_801E4474, sizeof D_801E4474) == 0);
}

int main(void)
{
    for (u16 key = 1; key <= 13; ++key)
        for (u16 route = 0; route < 44; ++route)
            check_selection(key, route);
    check_without_actor();
    return 0;
}
