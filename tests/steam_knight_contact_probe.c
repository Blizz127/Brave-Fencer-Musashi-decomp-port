#define _GNU_SOURCE
#include <assert.h>
#include <stddef.h>
#include <string.h>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "steam_knight_hit.h"

typedef SteamKnightPart *(*ContactFunction)(SteamKnightPart *, int, int, const void *, u16);
_Static_assert(_Generic(&func_8014C4AC, ContactFunction: 1, default: 0),
               "contact arguments and return type");
_Static_assert(sizeof(SteamKnightHitPosition) == 8, "contact position width");
_Static_assert(offsetof(SteamKnightPart, contact_parameter) == 98, "parameter offset");
_Static_assert(offsetof(SteamKnightPart, hit_position) == 124, "position offset");
_Static_assert(offsetof(SteamKnightPart, contact_tag) == 200, "tag offset");
_Static_assert(offsetof(SteamKnightPart, contact_frame) == 201, "frame offset");

SteamKnightPart *D_80126CD0;
s16 D_8012693A, D_801152B8;

static SteamKnightPart target, other, expected;
static const SteamKnightHitPosition position = {{83, 16, 91, 5, 47, 162, 32, 77}};
static const char *damage_calls;

static void reset_parts(void)
{
    memset(&target, 0x53, sizeof target);
    memset(&other, 0x29, sizeof other);
    target.active = 1;
    target.flags = 0x90a4;
    D_80126CD0 = &target;
    D_8012693A = -1234;
    D_801152B8 = 19047;
}

static void check_accept(int kind, int amount, const void *data, u16 parameter)
{
    memcpy(&expected, &target, sizeof expected);
    expected.hit_kind = (u16)kind;
    expected.damage = (s16)amount;
    expected.contact_parameter = parameter;
    expected.flags |= 1;
    memcpy(expected.hit_position.bytes, data, sizeof expected.hit_position.bytes);
    expected.contact_frame = (u8)D_8012693A;
    expected.contact_tag = (u8)D_801152B8;
    assert(func_8014C4AC(&target, kind, amount, data, parameter) == &target);
    assert(memcmp(&target, &expected, sizeof target) == 0);
}

static void check_filter(void)
{
    static const int allowed[] = {9, 17, 41, 10};
    static const int refused[] = {0, 8, 18, 40, 11, -1, 65545, -65526};
    size_t i;
    assert(func_8014C4AC(NULL, 9, 2, NULL, 0) == NULL);
    for (i = 0; i < sizeof allowed / sizeof *allowed; ++i) {
        reset_parts();
        check_accept(allowed[i], -321, &position, 0x901a);
    }

    reset_parts();
    target.active = 0;
    memcpy(&expected, &target, sizeof expected);
    assert(func_8014C4AC(&target, 9, 2, NULL, 1) == NULL);
    assert(memcmp(&target, &expected, sizeof target) == 0);

    target.active = 0x8000;
    memcpy(&expected, &target, sizeof expected);
    for (i = 0; i < sizeof refused / sizeof *refused; ++i) {
        assert(func_8014C4AC(&target, refused[i], 2, NULL, 1) == NULL);
        assert(memcmp(&target, &expected, sizeof target) == 0);
    }

    D_80126CD0 = &other;
    other.hit_position = position;
    SteamKnightPart before;
    memcpy(&before, &other, sizeof before);
    check_accept(65545, -65533, &other.hit_position, (u16)0x15623u);
    assert(memcmp(&other, &before, sizeof other) == 0);
    D_80126CD0 = NULL;
    check_accept(-1, 65535, &target.hit_position, 0xffff);
}

static void check_actor_inputs(void)
{
    size_t start, i;
    const size_t width = sizeof target.hit_position;
    for (start = 0; start <= sizeof target - width; ++start) {
        reset_parts();
        unsigned char *bytes = (unsigned char *)&target;
        for (i = 0; i < sizeof target; ++i)
            bytes[i] = (u8)(7u * i + 1u);
        target.active = 1;
        target.flags = 0x4900;
        memcpy(&expected, &target, sizeof target);

        /* The source sees the installed hit fields and the old frame stamps. */
        expected.flags |= 1;
        expected.hit_kind = 41;
        expected.damage = -614;
        expected.contact_parameter = 0xf738;
        u8 snapshot[sizeof expected.hit_position];
        memcpy(snapshot, (const unsigned char *)&expected + start, width);
        memcpy(expected.hit_position.bytes, snapshot, width);
        expected.contact_frame = (u8)D_8012693A;
        expected.contact_tag = (u8)D_801152B8;

        assert(func_8014C4AC(&target, 41, -614, bytes + start, 0xf738) == &target);
        assert(memcmp(&target, &expected, sizeof target) == 0);
    }
}

static void check_input_edges(void)
{
#if defined(__unix__) || defined(__APPLE__)
    long page_size = sysconf(_SC_PAGESIZE);
    assert(page_size > 0);
    size_t page = (size_t)page_size;
    unsigned char *region = mmap(NULL, 3 * page, PROT_NONE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(region != MAP_FAILED);
    assert(mprotect(region + page, page, PROT_READ | PROT_WRITE) == 0);
    SteamKnightHitPosition *first = (void *)(region + page);
    SteamKnightHitPosition *last = (void *)(region + 2 * page - sizeof *last);
    *first = position;
    *last = position;
    assert(mprotect(region + page, page, PROT_READ) == 0);
    reset_parts();
    assert(func_8014C4AC(&target, 2, 1, (const void *)region, 1) == NULL);
    check_accept(17, 22, first, 37);
    check_accept(10, -9, last, 61);
    assert(munmap(region, 3 * page) == 0);
#endif
}

void func_8016AA50(SteamKnightPart *part, int amount)
{
    assert(*damage_calls++ == 'D' && part == &target && amount == 13);
    expected.flags &= (u16)~1u;
    expected.hp = 38;
    assert(memcmp(part, &expected, sizeof expected) == 0);
}

void func_80186A04(SteamKnightPart *part)
{
    assert(*damage_calls++ == 'R' && part == &target);
    expected.reaction = 1;
    expected.timer = 8;
    assert(memcmp(part, &expected, sizeof expected) == 0);
}

void func_80185060(SteamKnightPart *part, int mode)
{
    (void)part; (void)mode;
    assert(0);
}

void func_80186B08(SteamKnightPart *part)
{
    (void)part;
    assert(0);
}

void func_8002D4C8(u16 command, u16 parameter)
{
    assert(*damage_calls++ == 'S' && command == 1267 && parameter == 0);
}

static void check_contact_then_hit(void)
{
    reset_parts();
    D_80126CD0 = &other;
    target.hp = 51;
    check_accept(2, 13, &position, 65);
    damage_calls = "DRS";
    func_801853EC(&target);
    assert(*damage_calls == '\0');
    assert(memcmp(&target, &expected, sizeof expected) == 0);
}

int main(void)
{
    check_filter();
    check_actor_inputs();
    check_input_edges();
    check_contact_then_hit();
    return 0;
}
