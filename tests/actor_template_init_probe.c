#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "steam_knight_resource.h"
#include "steam_knight_actor_dispose.h"

s32 func_8012C354(u8 *actor, u8 *record);
u8 D_80078EAE;

enum { ARENA_SIZE = 640, ACTOR = 176, SEPARATE = 32, MODEL = 512 };
enum { NO_MODEL, ALLOC_FAIL, ALLOC_OK };
typedef struct {
    unsigned record, model;
    u16 health;
    u8 mode, allocation, change_resource, change_flags;
} Example;

static const Example *test;
static u8 *arena;
static u8 before_allocate[ARENA_SIZE], before_last_service[ARENA_SIZE];
static unsigned encodes, allocations, disposals, attachments;
static s32 wanted_resource;

static u16 half(const u8 *p)
{
    return (u16)(p[0] | (u16)p[1] << 8);
}

static u32 word(const u8 *p)
{
    return (u32)half(p) | ((u32)half(p+2) << 16);
}

static void put_half(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static void put_word(u8 *p, u32 value)
{
    put_half(p, (u16)value);
    put_half(p+2, (u16)(value >> 16));
}

static u32 guest(unsigned offset)
{
    return 0x8513ac00u + offset;
}

u32 bfm_actor_template_address(const void *ptr)
{
    unsigned offset = encodes++ == 0 ? test->record : test->model;
    assert(ptr == arena+offset);
    assert((uintptr_t)ptr != guest(offset));
#if UINTPTR_MAX > UINT32_MAX
    assert((uintptr_t)ptr > UINT32_MAX);
#endif
    return guest(offset);
}

struct Record *func_8001CFDC(int first, int limit)
{
    assert(first == 38 && limit == 133);
    assert(allocations++ == 0 && encodes == 1);
    assert(memcmp(arena, before_allocate, ARENA_SIZE) == 0);
    if (test->change_resource) put_word(arena+test->record+4, 0x8a32c179u);
    return test->allocation == ALLOC_FAIL ? NULL : (struct Record *)(arena + test->model);
}

s32 func_8012CAE4(u8 *actor)
{
    assert(actor == arena+ACTOR && allocations == 1 && encodes == 1);
    assert(disposals++ == 0 && attachments == 0);
    assert(memcmp(arena, before_last_service, ARENA_SIZE) == 0);
    actor[0] = 0;
    return -1;
}

void func_8001C214(void *model, s32 resource)
{
    assert(model == arena+test->model && allocations == 1 && encodes == 2);
    assert(attachments++ == 0 && disposals == 0 && resource == wanted_resource);
    assert(memcmp(arena, before_last_service, ARENA_SIZE) == 0);
    if (test->change_flags) put_word(arena+test->record+8, 0x8e340712u);
}

static void check_case(const Example *item)
{
    _Alignas(16) u8 memory[ARENA_SIZE], expect[ARENA_SIZE];
    u8 *record = memory+item->record;
    test = item;
    arena = memory;
    D_80078EAE = item->mode;
    encodes = allocations = disposals = attachments = 0;
    for (unsigned i = 0; i < ARENA_SIZE; ++i) memory[i] = (u8)(0x5b + i*37u);
    put_half(record, item->health);
    put_half(record+2, 0x91a4u);
    put_word(record+4, item->allocation == NO_MODEL ? 0 : 0x821c5430u);
    put_word(record+8, item->change_flags ? 0x91u : 0);
    memcpy(expect, memory, sizeof expect);
    u8 *actor = expect+ACTOR;
    u8 *r = expect+item->record;
    put_word(actor+0x78, guest(item->record));

    u16 bits = half(r);
    /* The scaled branch rounds one and a half times signed health toward zero. */
    s32 value = bits & 0x8000u ? (s32)bits - 65536 : (s32)bits;
    s32 health = item->mode ? (s32)bits : value + value / 2;
    put_half(actor+0x76, (u16)health);
    put_half(actor+0x5c, half(r+2));
    int asks_for_model = word(r+4) != 0;
    int succeeds = !asks_for_model || item->allocation != ALLOC_FAIL;
    if (asks_for_model) {
        memcpy(before_allocate, expect, sizeof expect);
        if (item->change_resource) put_word(r+4, 0x8a32c179u);
        put_word(actor+0x20, succeeds ? guest(item->model) : 0);
        memcpy(before_last_service, expect, sizeof expect);
        if (succeeds) {
            wanted_resource = (s32)word(r+4);
            if (item->change_flags) put_word(r+8, 0x8e340712u);
        } else {
            actor[0] = 0;
        }
    }
    if (succeeds && word(r+8)) put_word(actor+0x58, word(r+8) | 0x60000000u);

    u32 guard20 = word(memory+ACTOR+0x24), guard78 = word(memory+ACTOR+0x7c);
    assert(func_8012C354(memory+ACTOR, record) == succeeds);
    /* Templates can overlap the guards themselves. In other cases both
     * four-byte neighbors must retain all their original bytes. */
    if (item->record == SEPARATE) {
        assert(word(memory+ACTOR+0x24) == guard20);
        assert(word(memory+ACTOR+0x7c) == guard78);
    }
    assert(memcmp(memory, expect, sizeof memory) == 0);
    assert(encodes == 1u + (asks_for_model && succeeds));
    assert(allocations == (unsigned)asks_for_model);
    assert(disposals == (unsigned)!succeeds);
    assert(attachments == (unsigned)(asks_for_model && succeeds));
}

int main(void)
{
    static const Example cases[] = {
        {SEPARATE, MODEL, 0xfffd, 0, NO_MODEL, 0, 0},
        {SEPARATE, MODEL, 0x8001, 0, ALLOC_FAIL, 1, 1},
        {SEPARATE, MODEL, 0x7fff, 0, ALLOC_OK, 1, 1},
        {SEPARATE, MODEL, 0xfffd, 1, ALLOC_OK, 0, 0},
        {SEPARATE, SEPARATE, 0x8000, 255, ALLOC_OK, 1, 1},
        /* Model slot and resource share a word. Its stored guest address is
         * the resource consumed by attachment, even after allocator writes. */
        {ACTOR+0x1c, ACTOR, 5, 1, ALLOC_OK, 1, 0},
        /* Installing the template replaces its own health and flags. */
        {ACTOR+0x78, MODEL, 7, 0, ALLOC_OK, 1, 0},
        /* Installing flags changes the template's final resource flags. */
        {ACTOR+0x54, MODEL, 0x8043, 0, NO_MODEL, 0, 0},
        /* The final template flags now occupy the actor template-handle word. */
        {ACTOR+0x70, MODEL, 0x8001, 0, ALLOC_OK, 1, 0}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i)
        check_case(&cases[i]);
    return 0;
}
