#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "steam_knight_hit.h"
#include "steam_knight_reaction.h"

s32 D_801E43A0;
u32 D_801E43CC[16];
u16 D_801E4BBC;

enum { TARGET, LINK, BODY, PEER_EIGHT, PEER_TEN, NEW_LINK, ACTORS };
static const u32 address[ACTORS] = {
    0xbd340710u, 0xbd340c30u, 0xbd340e40u,
    0xbd341250u, 0xbd341a60u, 0xbd341e70u
};

typedef struct {
    s16 first_variant, variant;
    int mode;
    s16 prior_cause;
    u16 inhibit_count;
    int redirect;
    const char *calls;
} BreakCase;

static const BreakCase *test;
static SteamKnightPart *actors;
static SteamKnightPart initial[ACTORS];
static const char *next_call;

static void record(char call)
{
    assert(*next_call == call);
    ++next_call;
}

static void check_common_progress(const SteamKnightPart *part)
{
    assert(part == &actors[TARGET]);
    assert(part->reaction == 0);
    assert(part->field_dc == 1);
    assert(part->break_cause == (test->prior_cause ? test->prior_cause : (s16)test->mode));
}

void func_80187280(SteamKnightPart *part, u16 key)
{
    record('D');
    assert(part == &actors[TARGET]);
    assert(memcmp(part, &initial[TARGET], sizeof *part) == 0);
    assert(key == (u16)(test->first_variant + 1));
    /* The caller must branch on the attachment's variant after display returns. */
    part->variant = test->variant;
}

SteamKnightPart *bfm_steam_knight_break_actor(u32 token)
{
    check_common_progress(&actors[TARGET]);
    for (int i = LINK; i < ACTORS; ++i) {
        if (token == address[i]) {
            record(i == BODY ? 'B' : i == PEER_EIGHT || i == PEER_TEN ? 'P' : 'L');
            return &actors[i];
        }
    }
    assert(0 && "unknown guest actor address");
    return NULL;
}

void func_80189F64(void)
{
    record('A');
    check_common_progress(&actors[TARGET]);
    assert(actors[LINK].phase_flags == (initial[LINK].phase_flags | 0x2000u));
    assert(actors[TARGET].break_delay == initial[TARGET].break_delay);
}

void func_801876B0(SteamKnightPart *part)
{
    record('S');
    check_common_progress(part);
    assert(part->break_delay == 24 && part->field_ae == initial[TARGET].field_ae);
}

void func_80187664(SteamKnightPart *part)
{
    record('N');
    check_common_progress(part);
    assert(part->break_delay == 16 && part->field_ae == initial[TARGET].field_ae);
    assert(actors[LINK].break_count == initial[LINK].break_count);
    if (test->variant == 8) assert(actors[PEER_EIGHT].flags == 0x8800);
    if (test->variant == 10) assert(actors[PEER_TEN].flags == 0x8800);
    if (test->variant == 9) assert(actors[BODY].phase_flags == (initial[BODY].phase_flags | 0x20000u));
    if (test->variant == 11) assert(actors[BODY].phase_flags == (initial[BODY].phase_flags | 0x40000u));
    if (test->redirect) {
        D_801E4BBC = 0;
        part->linked_address = address[NEW_LINK];
    }
}

void func_8012AD44(u8 *raw_actor, u16 state)
{
    record('T');
    SteamKnightPart *part = &actors[TARGET];
    assert(raw_actor == (u8 *)part && state == 2);
    check_common_progress(part);
    assert(part->field_ae == -1);
    assert(part->break_delay == (test->variant == 3 ? 24 : 16));
    if (test->variant != 3 && !D_801E4BBC) {
        int link = test->redirect ? NEW_LINK : LINK;
        assert(actors[link].break_count == (u16)(initial[link].break_count + 1));
    }
}

static void check_one(const BreakCase *which)
{
    SteamKnightPart objects[ACTORS], expected[ACTORS];
    test = which;
    next_call = test->calls;
    actors = objects;
    for (int i = 0; i < ACTORS; ++i) {
        memset(&actors[i], 37 + 3*i, sizeof actors[i]);
#if UINTPTR_MAX > UINT32_MAX
        assert((uintptr_t)&actors[i] > UINT32_MAX);
#endif
        assert((uintptr_t)&actors[i] != address[i]);
        actors[i].phase_flags = 0x98510038u;
        actors[i].flags = 0x9127u;
    }
    memset(D_801E43CC, 0xac, sizeof D_801E43CC);
    D_801E43CC[9] = address[PEER_EIGHT];
    D_801E43CC[11] = address[PEER_TEN];
    D_801E4BBC = test->inhibit_count;
    actors[TARGET].variant = test->first_variant;
    actors[TARGET].break_cause = test->prior_cause;
    actors[TARGET].linked_address = address[LINK];
    actors[TARGET].body_address = address[BODY];
    actors[LINK].break_count = 65535;
    actors[NEW_LINK].break_count = 52;
    memcpy(initial, actors, sizeof initial);
    memcpy(expected, actors, sizeof expected);

    expected[TARGET].variant = test->variant;
    expected[TARGET].reaction = 0;
    expected[TARGET].field_dc = 1;
    expected[TARGET].break_cause = test->prior_cause ? test->prior_cause : (s16)test->mode;
    expected[TARGET].break_delay = test->variant == 3 ? 24 : 16;
    expected[TARGET].field_ae = -1;
    if (test->variant == 3) expected[LINK].phase_flags |= 0x2000u;
    if (test->variant == 8) expected[PEER_EIGHT].flags = 0x8800u;
    if (test->variant == 10) expected[PEER_TEN].flags = 0x8800u;
    if (test->variant == 9) expected[BODY].phase_flags |= 0x20000u;
    if (test->variant == 11) expected[BODY].phase_flags |= 0x40000u;
    if (test->redirect) expected[TARGET].linked_address = address[NEW_LINK];
    if (test->variant != 3 && (test->redirect || !test->inhibit_count))
        ++expected[test->redirect ? NEW_LINK : LINK].break_count;

    func_80185060(&actors[TARGET], test->mode);
    assert(*next_call == '\0');
    assert(memcmp(actors, expected, sizeof expected) == 0);
}

enum { REACTION_RECORDS = 5 };
u16 D_80185B4C[32768 + 14];
SteamKnightReaction D_801961B8[REACTION_RECORDS], D_801961E0[REACTION_RECORDS];

_Static_assert(sizeof(SteamKnightReaction) == 8, "reaction record stride");
_Static_assert(offsetof(SteamKnightReaction, id) == 6, "reaction ID offset");

static void check_reaction_step(void (*operation)(SteamKnightPart *), int variant,
                                u16 route, u32 mask, const s16 before[5],
                                const s16 after[5])
{
    SteamKnightPart part, saved;
    SteamKnightReaction expected[REACTION_RECORDS], other_expected[REACTION_RECORDS];
    SteamKnightReaction *selected = route < 22 ? D_801961B8 : D_801961E0;
    SteamKnightReaction *other = route < 22 ? D_801961E0 : D_801961B8;

    memset(&part, 0x65, sizeof part);
    part.variant = (s16)variant;
    memcpy(&saved, &part, sizeof part);
    if (variant < 14) D_80185B4C[32768 + variant] = route;
    D_801E43A0 = (s32)mask;
    for (int i = 0; i < REACTION_RECORDS; ++i) {
        selected[i] = (SteamKnightReaction){(s16)(-93+i), (s16)(120+i), (s16)(79-i), before[i]};
        other[i] = (SteamKnightReaction){(s16)(-19-i), (s16)(211+i), (s16)(-75+i), (s16)(i+1)};
    }
    memcpy(expected, selected, sizeof expected);
    memcpy(other_expected, other, sizeof other_expected);
    for (int i = 0; i < REACTION_RECORDS; ++i) expected[i].id = after[i];
    operation(&part);
    assert(!memcmp(&saved, &part, sizeof part));
    assert(!memcmp(expected, selected, sizeof expected));
    assert(!memcmp(other_expected, other, sizeof other_expected));
    assert((u32)D_801E43A0 == mask);
}

#define IDS(a, b, c, d, e) ((const s16[5]){a, b, c, d, e})

static void check_reaction_lists(void)
{
    /* Only the first duplicate is removed. The fifth ID fills the fourth. */
    check_reaction_step(func_80186B08, 10, 11, 1u << 11,
                        IDS(2,11,8,11,27), IDS(2,8,11,27,27));
    check_reaction_step(func_80186B08, SHRT_MIN, 23, 2,
                        IDS(0,1,3,4,-1), IDS(0,3,4,-1,-1));
    check_reaction_step(func_80186B08, 13, 42, 0,
                        IDS(1,3,7,20,-7), IDS(1,3,7,-7,-7));
    /* A match in the fifth record is outside the searchable list. */
    check_reaction_step(func_80186B08, 0, 8, 0,
                        IDS(1,3,5,7,8), IDS(1,3,5,7,8));
    check_reaction_step(func_80186B08, 14, 22, 0,
                        IDS(0,3,7,9,16), IDS(0,3,7,9,16));
    check_reaction_step(func_80186A04, 14, 22, 0,
                        IDS(1,3,7,9,16), IDS(1,3,7,9,16));

    /* An enabled mask bit blocks insertion but still permits removal. */
    check_reaction_step(func_80186A04, SHRT_MIN, 23, 2,
                        IDS(0,3,4,-1,-1), IDS(0,3,4,-1,-1));

    /* Reuse the actual updated list for a different part after removal. */
    check_reaction_step(func_80186B08, 4, 30, 0,
                        IDS(1,4,8,12,-1), IDS(1,4,12,-1,-1));
    D_80185B4C[32768 + 7] = 31;
    SteamKnightPart next = {0};
    next.variant = 7;
    func_80186A04(&next);
    const s16 expected[] = {1,4,9,12,-1};
    for (int i = 0; i < REACTION_RECORDS; ++i)
        assert(D_801961E0[i].id == expected[i]);
}

int main(void)
{
    static const BreakCase cases[] = {
        {3, 3, -7, 0, 0, 0, "DLAST"},
        {3, 3, 0, 412, 0, 0, "DLAT"},
        {8, 8, 1, 742, 0, 0, "DPNLT"},
        {10, 10, 0, 0, 0, 0, "DPLT"},
        {9, 9, -2, 0, 1, 0, "DBNT"},
        {11, 11, 1, 0, 0, 0, "DBNLT"},
        {3, 10, 65536, 0, 1, 1, "DPNLT"},
        {-1, -1, 0, -9, 0, 0, "DLT"},
        {16, 16, 0, 0, 1, 0, "DT"}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i)
        check_one(&cases[i]);
    check_reaction_lists();
    return 0;
}
