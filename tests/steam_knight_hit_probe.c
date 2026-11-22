#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "steam_knight_hit.h"

_Static_assert(offsetof(SteamKnightPart, timer) == 28, "timer offset");
_Static_assert(offsetof(SteamKnightPart, flags) == 92, "flags offset");
_Static_assert(offsetof(SteamKnightPart, hit_kind) == 94, "hit kind offset");
_Static_assert(offsetof(SteamKnightPart, damage) == 96, "damage offset");
_Static_assert(offsetof(SteamKnightPart, hp) == 118, "HP offset");
_Static_assert(offsetof(SteamKnightPart, reaction) == 193, "reaction offset");

enum { KEEP_HP = 32768, KEEP_KIND = -1 };

typedef struct {
    s16 hp, damage, debited_hp;
    u16 flags, kind;
    int hp_from_damage, kind_from_damage, kind_from_action;
    const char *calls;
} HitCase;

/* D applies damage, R reacts, B begins defeat, F finishes defeat, S plays sound. */
static const HitCase cases[] = {
    {51, 6, 45, 0x579b, 12, KEEP_HP, KEEP_KIND, KEEP_KIND, "DRS"},
    {6, 6, 0, 0xa5a4, 29, KEEP_HP, KEEP_KIND, KEEP_KIND, "DBFS"},
    {2, 7, -5, 0xffff, 11, KEEP_HP, KEEP_KIND, 29, "DBF"},
    {13, 0, 13, 0x0000, 5, KEEP_HP, 29, KEEP_KIND, "DR"},
    {0, -1, 1, 0x8001, 29, KEEP_HP, KEEP_KIND, 13, "DRS"},
    {32767, -1, -32768, 0x0001, 7, KEEP_HP, KEEP_KIND, KEEP_KIND, "DBFS"},
    {-32768, 1, 32767, 0xfffe, 19, KEEP_HP, KEEP_KIND, KEEP_KIND, "DRS"},
    {-32768, -32768, 0, 0x0101, 7, 1, KEEP_KIND, 29, "DR"},
    {9, 3, 6, 0x0801, 9, -2, 29, KEEP_KIND, "DBFS"}
};

static const HitCase *test;
static const char *next_call;
static SteamKnightPart part, expected;

static void observe(char call, const SteamKnightPart *actual)
{
    assert(actual == &part);
    assert(*next_call == call);
    ++next_call;
    assert(memcmp(actual, &expected, sizeof expected) == 0);
}

void func_8016AA50(SteamKnightPart *actual, int amount)
{
    assert(amount == test->damage);
    expected.hp = test->debited_hp;
    expected.flags &= (u16)~1u;
    observe('D', actual);

    if (test->hp_from_damage != KEEP_HP)
        actual->hp = expected.hp = (s16)test->hp_from_damage;
    if (test->kind_from_damage != KEEP_KIND)
        actual->hit_kind = expected.hit_kind = (u16)test->kind_from_damage;
}

static void change_kind(SteamKnightPart *actual)
{
    if (test->kind_from_action != KEEP_KIND)
        actual->hit_kind = expected.hit_kind = (u16)test->kind_from_action;
}

void func_80186A04(SteamKnightPart *actual)
{
    expected.reaction = 1;
    expected.timer = 8;
    observe('R', actual);
    change_kind(actual);
}

void func_80185060(SteamKnightPart *actual, int mode)
{
    assert(mode == 1);
    expected.hit_kind = 0;
    observe('B', actual);
    actual->hit_kind = expected.hit_kind = 71;
}

void func_80186B08(SteamKnightPart *actual)
{
    observe('F', actual);
    change_kind(actual);
}

void func_8002D4C8(u16 command, u16 argument)
{
    assert(command == 1267 && argument == 0);
    observe('S', &part);
}

int main(void)
{
    size_t i;
    for (i = 0; i < sizeof cases / sizeof *cases; ++i) {
        test = &cases[i];
        next_call = test->calls;
        memset(&part, 0x5a, sizeof part);
        part.timer = -42;
        part.hp = test->hp;
        part.damage = test->damage;
        part.flags = test->flags;
        part.hit_kind = test->kind;
        memcpy(&expected, &part, sizeof expected);
        func_801853EC(&part);
        assert(*next_call == '\0');
        assert(memcmp(&part, &expected, sizeof part) == 0);
    }
    return 0;
}
