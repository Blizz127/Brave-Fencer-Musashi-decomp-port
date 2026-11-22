#define _GNU_SOURCE
#include <assert.h>
#include <stddef.h>
#include <string.h>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "steam_knight_effect.h"

typedef SteamKnightEffect *(*Submit)(int, u32, int, int, s16, int, int);
typedef SteamKnightEffect *(*Insert)(const SteamKnightEffectRequest *);
_Static_assert(_Generic(&func_80146A6C, Submit: 1, default: 0), "submit ABI");
_Static_assert(_Generic(&func_80146AFC, Insert: 1, default: 0), "insert ABI");

#define OFFSET(type, field, offset) \
    _Static_assert(offsetof(type, field) == offset, #type "." #field)
_Static_assert(sizeof(SteamKnightEffectRequest) == 20, "descriptor size");
OFFSET(SteamKnightEffectRequest, kind, 0);
OFFSET(SteamKnightEffectRequest, x, 2);
OFFSET(SteamKnightEffectRequest, y, 4);
OFFSET(SteamKnightEffectRequest, z, 6);
OFFSET(SteamKnightEffectRequest, owner_address, 8);
OFFSET(SteamKnightEffectRequest, mode, 12);
OFFSET(SteamKnightEffectRequest, amount, 16);
_Static_assert(sizeof(SteamKnightEffect) == 88, "slot stride");
OFFSET(SteamKnightEffect, kind, 0);
OFFSET(SteamKnightEffect, x, 6);
OFFSET(SteamKnightEffect, y, 10);
OFFSET(SteamKnightEffect, z, 14);
OFFSET(SteamKnightEffect, mode, 44);
OFFSET(SteamKnightEffect, amount, 48);
OFFSET(SteamKnightEffect, owner_address, 52);

enum { SLOT_COUNT = 30 };
SteamKnightEffect D_8011D030[SLOT_COUNT];
static SteamKnightEffect expected[SLOT_COUNT];

static void expect_fields(SteamKnightEffect *slot, const SteamKnightEffectRequest *r)
{
    slot->kind = r->kind;
    slot->x = r->x;
    slot->y = r->y;
    slot->z = r->z;
    slot->owner_address = r->owner_address;
    slot->mode = r->mode;
    slot->amount = r->amount;
}

static void check_submit_and_capacity(void)
{
    memset(D_8011D030, 0x57, sizeof D_8011D030);
    for (int i = 0; i < SLOT_COUNT; ++i)
        D_8011D030[i].kind = 0;
    memcpy(expected, D_8011D030, sizeof expected);

    for (int i = 0; i < SLOT_COUNT; ++i) {
        SteamKnightEffectRequest r = {
            .kind = (u16)(37 + i), .x = (s16)(-1200 - i), .y = (s16)(800 + i),
            .z = (s16)(-32768 + i), .owner_address = 0xe132a476u - (u32)i,
            .mode = 0x23456700 + i, .amount = -120400 - i,
        };
        expect_fields(&expected[i], &r);
        /* X, Y and kind are int inputs with only their low half installed. */
        assert(func_80146A6C(65536 + r.kind, r.owner_address, 65536 + r.x,
                             r.y - 65536, r.z, r.mode, r.amount) == &D_8011D030[i]);
        assert(memcmp(D_8011D030, expected, sizeof expected) == 0);
    }

    /* Reading a request must be unnecessary once the final slot is occupied. */
    assert(func_80146AFC(NULL) == NULL);
#if defined(__unix__) || defined(__APPLE__)
    long page_size = sysconf(_SC_PAGESIZE);
    assert(page_size > 0);
    void *unreadable = mmap(NULL, (size_t)page_size, PROT_NONE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(unreadable != MAP_FAILED);
    assert(func_80146AFC(unreadable) == NULL);
    assert(munmap(unreadable, (size_t)page_size) == 0);
#endif
    assert(func_80146A6C(3, 14u, 12, 5, -17, 8, 27) == NULL);
    assert(memcmp(D_8011D030, expected, sizeof expected) == 0);
}

static void check_zero_kind_reuses_hole(void)
{
    const int hole = 12;
    SteamKnightEffectRequest r = {0, 97, -135, 311, 0xde40a71cu, -7184, 100951};
    memset(D_8011D030, 0x67, sizeof D_8011D030);
    D_8011D030[hole].kind = 0;
    D_8011D030[hole + 2].kind = 0;
    memcpy(expected, D_8011D030, sizeof expected);
    expect_fields(&expected[hole], &r);
    assert(func_80146AFC(&r) == &D_8011D030[hole]);
    assert(memcmp(D_8011D030, expected, sizeof expected) == 0);
    r.kind = 4;
    expect_fields(&expected[hole], &r);
    assert(func_80146AFC(&r) == &D_8011D030[hole]);
    assert(memcmp(D_8011D030, expected, sizeof expected) == 0);
}

static void check_overlapping_request(void)
{
    const int hole = 7;
    SteamKnightEffectRequest r = {11, -309, 275, -128, 0x317b8da9u, -527, 81291};
    unsigned char *slot = (unsigned char *)&D_8011D030[hole];
    size_t start = offsetof(SteamKnightEffect, owner_address)
                 - offsetof(SteamKnightEffectRequest, mode);
    memset(D_8011D030, 0x67, sizeof D_8011D030);
    D_8011D030[hole].kind = 0;
    memcpy(slot + start, &r, sizeof r);
    memcpy(expected, D_8011D030, sizeof expected);

    /* Storing the owner overwrites the request's mode before mode is read. */
    r.mode = (s32)r.owner_address;
    expect_fields(&expected[hole], &r);
    assert(func_80146AFC((const SteamKnightEffectRequest *)(slot + start))
           == &D_8011D030[hole]);
    assert(memcmp(D_8011D030, expected, sizeof expected) == 0);
}

int main(void)
{
    check_submit_and_capacity();
    check_zero_kind_reuses_hole();
    check_overlapping_request();
    return 0;
}
