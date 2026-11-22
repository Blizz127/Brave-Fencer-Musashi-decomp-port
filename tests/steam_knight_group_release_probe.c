#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "steam_knight_hit.h"

extern void func_8018AAAC(void);
enum { SLOTS = 12, AXES = 3, MODEL_WORDS = 24, FIRST_TRANSLATION = 18 };
#define GUEST_BASE 0xd7310000u

u32 D_801E4A58[SLOTS];
static u32 expected_slots[SLOTS];

typedef struct {
    u32 prefix[32];
    SteamKnightPart actor[SLOTS];
    u32 model[SLOTS + 1][MODEL_WORDS];
    u32 trailer[8];
} Arena;

static Arena *live;
static Arena expected;
static const unsigned *order;
static size_t visits, completed;
static unsigned stage;
static int changing;

static u32 address_of(const void *object)
{
    return GUEST_BASE + (u32)((const u8 *)object - (const u8 *)live);
}

void *bfm_tower_spawn_pointer(u32 address)
{
    u32 offset = address - GUEST_BASE;
    assert(offset < sizeof *live);
    return (u8 *)live + offset;
}

static void check_memory(void)
{
    assert(memcmp(live, &expected, sizeof expected) == 0);
    assert(memcmp(D_801E4A58, expected_slots, sizeof expected_slots) == 0);
}

static unsigned current(const void *effect, unsigned expected_stage)
{
    assert(completed < visits && stage == expected_stage);
    unsigned i = order[completed];
    assert(effect == &live->actor[i]);
    return i;
}

static void set_slot(unsigned index, u32 guest)
{
    D_801E4A58[index] = expected_slots[index] = guest;
}

void bfm_steam_knight_core_attach(SteamKnightPart *effect, u32 owner, int joint)
{
    unsigned i = current(effect, 0);
    check_memory();
    assert(owner == expected.actor[i].body_address);
    assert(joint == expected.actor[i].break_count);

    if (!(changing && i == 6)) {
        u32 model = address_of(live->model[i]);
        effect->model_address = expected.actor[i].model_address = model;
    }
    effect->state = expected.actor[i].state = 0x43ab;
    if (changing && completed == 0) {
        set_slot(0, address_of(&live->actor[5]));
        set_slot(1, address_of(&live->actor[6]));
        set_slot(3, 0);
        live->actor[1].state = expected.actor[1].state = 2;
    }
    stage = 1;
}

void func_8012B23C(s32 *effect)
{
    unsigned i = current(effect, 1);
    SteamKnightPart *shadow = &expected.actor[i];
    const u8 *model = (const u8 *)&expected + (shadow->model_address - GUEST_BASE);
    for (unsigned axis = 0; axis < AXES; ++axis) {
        u32 word;
        memcpy(&word, model + (FIRST_TRANSLATION + axis) * sizeof word, sizeof word);
        u16 position = (u16)word;
        memcpy((u8 *)shadow + 6 + axis * 4, &position, sizeof position);
    }
    check_memory();

    SteamKnightPart *p = &live->actor[i];
    p->position_y = shadow->position_y = 16127;
    p->motion_x = shadow->motion_x = -21443583;
    p->motion_y = shadow->motion_y = 1993723;
    p->motion_z = shadow->motion_z = -483274;
    p->timer = shadow->timer = 731;
    if (changing && completed == 0) {
        /* The new slot points into the same guest arena as the actors.
           Its translations will read the first actor's motion after the setter. */
        u32 shared = address_of(&p->motion_x) - FIRST_TRANSLATION * sizeof(u32);
        live->actor[6].model_address = expected.actor[6].model_address = shared;
    }
    stage = 2;
}

void func_8012AD44(u8 *effect, u16 state)
{
    unsigned i = current(effect, 2);
    SteamKnightPart *p = &live->actor[i], *shadow = &expected.actor[i];
    shadow->motion_x = 1048576;
    shadow->motion_y = -2097152;
    shadow->motion_z = -262144;
    shadow->timer = 32;
    assert(state == 5);
    check_memory();
    p->state = shadow->state = state;
    if (changing && completed == 0) {
        p->state = shadow->state = 2;
        p->motion_x = shadow->motion_x = 0x47001123;
        p->motion_y = shadow->motion_y = -0x12301ab7;
        p->motion_z = shadow->motion_z = 0x008a8021;
        live->actor[4].state = expected.actor[4].state = 0x102;
    }
    ++completed;
    stage = 0;
}

static void initialize(void)
{
    memset(live, 0xa7, sizeof *live);
    memset(D_801E4A58, 0, sizeof D_801E4A58);
    for (unsigned i = 0; i < SLOTS; ++i) {
        SteamKnightPart *p = &live->actor[i];
        p->state = 2;
        p->body_address = 0xe06a1020u + 0x934u * i;
        p->break_count = (u16)(i == 0 ? 0xffffu : 0x8067u + 47u * i);
        p->model_address = address_of(live->model[SLOTS]);
        live->model[i][18] = 0x12558000u + i * 13u;
        live->model[i][19] = 0xed170005u + i * 19u;
        live->model[i][20] = 0x7fffabc3u - i * 23u;
    }
    completed = stage = changing = 0;
}

static void run(const unsigned *expected_order, size_t count)
{
    order = expected_order;
    visits = count;
    memcpy(&expected, live, sizeof expected);
    memcpy(expected_slots, D_801E4A58, sizeof expected_slots);
    func_8018AAAC();
    assert(completed == visits && stage == 0);
    check_memory();
}

int main(void)
{
    Arena arena;
    live = &arena;
#if defined(__linux__) && UINTPTR_MAX > UINT32_MAX
    assert((uintptr_t)&arena.actor[0] > UINT32_MAX);
#endif
    initialize();
    run(NULL, 0);

    initialize();
    D_801E4A58[0] = address_of(&live->actor[0]);
    live->actor[0].state = 0x102;
    D_801E4A58[3] = D_801E4A58[6] = address_of(&live->actor[8]);
    D_801E4A58[10] = address_of(&live->actor[9]);
    live->actor[9].state = 0;
    D_801E4A58[11] = address_of(&live->actor[1]);
    const unsigned sparse[] = {8, 1};
    run(sparse, 2);

    initialize();
    unsigned full[SLOTS];
    for (unsigned i = 0; i < SLOTS; ++i) {
        full[i] = SLOTS - 1 - i;
        D_801E4A58[i] = address_of(&live->actor[full[i]]);
    }
    run(full, SLOTS);

    initialize();
    changing = 1;
    live->actor[1].state = 0x102;
    D_801E4A58[0] = D_801E4A58[5] = address_of(&live->actor[0]);
    D_801E4A58[1] = address_of(&live->actor[2]);
    D_801E4A58[2] = address_of(&live->actor[1]);
    D_801E4A58[3] = address_of(&live->actor[3]);
    D_801E4A58[4] = address_of(&live->actor[4]);
    const unsigned changed[] = {0, 6, 1, 0};
    run(changed, 4);
    return 0;
}
