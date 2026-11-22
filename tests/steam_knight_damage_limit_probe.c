#include "steam_knight_core.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

u16 D_801E4BBC;
s16 D_80195B24[8];

static int signed_half(unsigned bits)
{
    bits &= 65535u;
    return bits < 32768u ? (int)bits : (int)bits - 65536;
}

static int half_floor(int value)
{
    return value >= 0 ? value / 2 : -((-value + 1) / 2);
}

static void check_damage(SteamKnightPart *actor, int amount, int limit, int bypass)
{
    SteamKnightPart before;
    int current = signed_half((unsigned)(u16)actor->hp - (unsigned)(u16)actor->damage);
    int target = signed_half((unsigned)(current - amount));
    int floor = bypass || target >= limit ? target : limit;
    int expected = signed_half((unsigned)(current - floor));
    memcpy(&before, actor, sizeof before);
    assert(func_80185A88(actor, (s16)amount) == expected);
    assert(memcmp(&before, actor, sizeof before) == 0);
}

int main(void)
{
    SteamKnightPart actor;
    static const u32 flags[] = {0, 0x40000u, 0x800u, 0x80000u, 0x80800u, 0xffffffffu};
    static const unsigned halfwords[] = {0, 1, 2, 95, 96, 127, 128, 191, 32767, 32768, 65535};
    memset(&actor, 0xa5, sizeof actor);
    _Static_assert(offsetof(SteamKnightPart, damage) == 0x60, "damage");
    _Static_assert(offsetof(SteamKnightPart, hp) == 0x76, "hp");
    _Static_assert(offsetof(SteamKnightPart, phase_flags) == 0xe8, "flags");
    for (unsigned i = 0; i < 65536u; ++i) {
        unsigned round = i & 7u;
        int table = signed_half(i);
        int limit = round == 2 ? 128 : half_floor(table);
        D_801E4BBC = (u16)round;
        D_80195B24[round] = (s16)table;
        actor.damage = (s16)signed_half(i * 173u);
        for (size_t n = 0; n < sizeof flags / sizeof *flags; ++n) {
            actor.phase_flags = flags[n];
            int bypass = (flags[n] & (round == 2 ? 0x800u : 0x80000u)) != 0;
            for (int delta = -1; delta <= 1; ++delta) {
                int incoming = signed_half((i ^ ((unsigned)n * 353u)) * 3955u);
                actor.hp = (s16)signed_half((unsigned)(actor.damage + incoming + limit + delta));
                check_damage(&actor, incoming, limit, bypass);
            }
        }
    }
    for (unsigned round = 0; round < 3; ++round) {
        D_801E4BBC = (u16)round;
        D_80195B24[round] = (s16)(round ? -197 : 397);
        int limit = round == 2 ? 128 : half_floor(D_80195B24[round]);
        actor.phase_flags = 0;
        for (size_t x = 0; x < sizeof halfwords / sizeof *halfwords; ++x) {
            actor.hp = (s16)signed_half(halfwords[x]);
            for (size_t d = 0; d < sizeof halfwords / sizeof *halfwords; ++d) {
                actor.damage = (s16)signed_half(halfwords[d]);
                for (size_t a = 0; a < sizeof halfwords / sizeof *halfwords; ++a)
                    check_damage(&actor, signed_half(halfwords[a]), limit, 0);
            }
        }
    }
    return 0;
}
