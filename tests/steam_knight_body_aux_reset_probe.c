#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "steam_knight_hit.h"

extern void func_80184C20(SteamKnightPart *actor);
extern void func_80186F58(SteamKnightPart *actor);

#define CHILD       0xb4a72100u
#define SELF        0x6f5ca000u
#define EARLY_BODY  0xd1834900u
#define NEXT_BODY   0x6920d100u
#define LATE_BODY   0xf9c67200u
#define ALIAS_BODY  0xa7d12800u

/* In the alias case the parent's link occupies the actor's second cleared word. */
typedef union {
    SteamKnightPart parent;
    struct {
        u32 padding[49];
        SteamKnightPart actor;
    } slot;
} Shared;

_Static_assert(offsetof(Shared, slot.actor) + 8 == offsetof(SteamKnightPart, linked_address),
               "parent link aliases an actor word");
_Static_assert(offsetof(SteamKnightPart, field_fc) == 0xfc, "countdown offset");
_Static_assert(sizeof(((SteamKnightPart *)0)->field_fc) == 2, "countdown width");

typedef struct {
    u32 before[4];
    Shared shared;
    SteamKnightPart child, early_body, next_body, late_body, zero_body;
    u32 after[4];
} Fixture;

typedef struct {
    u32 previous_child, late_body, attachment;
} Case;

static Fixture *live;
static Fixture expected;
static Case input;
static const char *steps;

static SteamKnightPart *actor(Fixture *f)
{
    return &f->shared.slot.actor;
}

static void check_step(char step)
{
    assert(*steps == step);
    ++steps;
    assert(memcmp(live, &expected, sizeof expected) == 0);
}

void *bfm_tower_spawn_pointer(u32 address)
{
    struct {
        u32 base;
        SteamKnightPart *part;
    } mappings[] = {
        {CHILD, &live->child}, {SELF, actor(live)}, {EARLY_BODY, &live->early_body},
        {NEXT_BODY, &live->next_body}, {LATE_BODY, &live->late_body},
        {ALIAS_BODY, &live->shared.parent}, {0, &live->zero_body},
    };
    for (size_t i = 0; i < sizeof mappings / sizeof mappings[0]; ++i) {
        u32 offset = address - mappings[i].base;
        if (offset < sizeof(SteamKnightPart))
            return (u8 *)mappings[i].part + offset;
    }
    assert(0 && "unmapped guest address");
    return NULL;
}

void func_8012AD44(u8 *part, u16 state)
{
    assert(part == (u8 *)actor(live) && state == 1);
    if (input.previous_child == CHILD)
        expected.child.field_fc = 1;
    if (input.previous_child == SELF)
        actor(&expected)->field_fc = 1;
    actor(&expected)->linked_address = 0;
    actor(&expected)->display_record = 0x801963fcu;
    actor(&expected)->flags = 0x8c00;
    check_step('S');

    actor(live)->body_address = actor(&expected)->body_address = NEXT_BODY;
    actor(live)->linked_address = actor(&expected)->linked_address = 0x6382b080u;
    actor(live)->display_record = actor(&expected)->display_record = 0xea132014u;
    actor(live)->flags = actor(&expected)->flags = 0x6bd3;
    actor(live)->state = actor(&expected)->state = state;
}

void func_8012B200(void *part)
{
    assert(part == actor(live));
    check_step('C');
    actor(live)->body_address = actor(&expected)->body_address = input.late_body;
    memset((u8 *)part + 4, 0xd7, 12);
    memset((u8 *)actor(&expected) + 4, 0xd7, 12);
    if (input.late_body == LATE_BODY)
        live->late_body.linked_address = expected.late_body.linked_address = input.attachment;
    if (input.late_body == CHILD)
        live->child.linked_address = expected.child.linked_address = input.attachment;
    if (input.late_body == 0)
        live->zero_body.linked_address = expected.zero_body.linked_address = input.attachment;
}

void bfm_steam_knight_core_attach(SteamKnightPart *part, u32 parent, int index)
{
    memset((u8 *)actor(&expected) + 4, 0, 12);
    assert(part == actor(live) && parent == input.attachment && index == 6);
    check_step('A');
    memset((u8 *)part + 4, 0x29, 12);
    memset((u8 *)actor(&expected) + 4, 0x29, 12);
    part->field_fc = actor(&expected)->field_fc = 0x47ac;
}

static void check_reset(Case c)
{
    input = c;
    steps = "SCA";
    memset(live, 0x9e, sizeof *live);
    actor(live)->linked_address = c.previous_child;
    actor(live)->body_address = EARLY_BODY;
    live->early_body.linked_address = 0x654e0b80u;
    live->next_body.linked_address = 0x74f25d30u;
    memcpy(&expected, live, sizeof expected);
    func_80184C20(actor(live));
    assert(*steps == '\0' && memcmp(live, &expected, sizeof expected) == 0);
}

static void check_countdown(void)
{
    static const u16 values[] = {0, 1, 0x0100, 0x00ff, 0x8001, 0xffff};
    steps = "";
    for (size_t i = 0; i < sizeof values / sizeof values[0]; ++i) {
        memset(live, 0xe7, sizeof *live);
        actor(live)->field_fc = values[i];
        memcpy(&expected, live, sizeof expected);
        actor(&expected)->field_fc = 0;
        func_80186F58(actor(live));
        assert(memcmp(live, &expected, sizeof expected) == 0);
    }
}

int main(void)
{
    Fixture storage;
    live = &storage;
#if defined(__linux__) && UINTPTR_MAX > UINT32_MAX
    assert((uintptr_t)actor(live) > UINT32_MAX);
#endif
    check_countdown();
    check_reset((Case){0, LATE_BODY, 0x94e85100u});
    check_reset((Case){CHILD, LATE_BODY, 0});
    check_reset((Case){SELF, ALIAS_BODY, 0});
    check_reset((Case){CHILD, CHILD, 0x13502140u});
    check_reset((Case){SELF, 0, 0xf6023500u});
    return 0;
}
