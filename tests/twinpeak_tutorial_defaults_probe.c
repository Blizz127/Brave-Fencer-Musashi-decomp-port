#include "message_defaults.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { CLEAR_WORDS = 648, GUARD_BYTES = 16 };

_Static_assert(CHAR_BIT == 8 && sizeof(unsigned short) == 2, "byte and halfword sizes");
_Static_assert(sizeof(TutorialHandleSlot) == 76, "slot size");
_Static_assert(sizeof(TutorialMessageRect) == 8, "rectangle size");

unsigned short D_80127C0C[CLEAR_WORDS];

typedef struct GuardedSlot {
    unsigned char before[GUARD_BYTES];
    TutorialHandleSlot slot;
    unsigned char after[GUARD_BYTES];
} GuardedSlot;

static GuardedSlot actual;
static GuardedSlot expected;
static unsigned short expected_words[CLEAR_WORDS];
static unsigned short expected_x;
static unsigned short expected_y;
static unsigned int rect_calls;
static unsigned int trial;
static const char *operation;

static void check(int ok, const char *what)
{
    if (!ok) {
        fprintf(stderr, "trial %u, %s: %s\n", trial, operation, what);
        exit(EXIT_FAILURE);
    }
}

static unsigned char *slot_bytes(GuardedSlot *guard)
{
    return (unsigned char *)&guard->slot;
}

static void store_half(unsigned char *p, unsigned short value)
{
    memcpy(p, &value, sizeof(value));
}

static void check_memory(const char *when)
{
    if (memcmp(&actual, &expected, sizeof(actual)) != 0) {
        const unsigned char *got = (const unsigned char *)&actual;
        const unsigned char *want = (const unsigned char *)&expected;
        size_t i;
        for (i = 0; i < sizeof(actual); ++i) {
            if (got[i] != want[i]) {
                fprintf(stderr, "%s, guarded slot byte %zu: %02x != %02x\n",
                        when, i, (unsigned int)got[i], (unsigned int)want[i]);
                break;
            }
        }
        check(0, "slot or guard differs");
    }
    if (memcmp(D_80127C0C, expected_words, sizeof(expected_words)) != 0) {
        size_t i;
        for (i = 0; i < CLEAR_WORDS; ++i) {
            if (D_80127C0C[i] != expected_words[i]) {
                fprintf(stderr, "%s, clear word %zu: %04x != %04x\n", when,
                        i, (unsigned int)D_80127C0C[i], (unsigned int)expected_words[i]);
                break;
            }
        }
        check(0, "clear area differs");
    }
}

static void prepare(unsigned short x, unsigned short y)
{
    unsigned char *base = (unsigned char *)&actual;
    size_t i;

    /* All starting bytes and clear words are nonzero, apart from explicit coordinates. */
    for (i = 0; i < sizeof(actual); ++i)
        base[i] = (unsigned char)(1 + ((i * 71 + trial * 37) % 254));
    store_half(slot_bytes(&actual) + 56, x);
    store_half(slot_bytes(&actual) + 58, y);
    for (i = 0; i < CLEAR_WORDS; ++i)
        D_80127C0C[i] = (unsigned short)(1 + ((i * 811u + trial * 509u) % 65535u));
    memcpy(&expected, &actual, sizeof(expected));
    memcpy(expected_words, D_80127C0C, sizeof(expected_words));
    expected_x = x;
    expected_y = y;
    rect_calls = 0;
}

static void expect_defaults_before_rect(void)
{
    /* Numeric slot offsets visible at the call boundary. Byte 13 changes later. */
    static const unsigned char zero_halves[] = {
        4, 6, 16, 18, 20, 22, 28, 48, 50, 52, 54, 68, 70, 72, 74
    };
    unsigned char *b = slot_bytes(&expected);
    size_t i;
    for (i = 0; i < sizeof(zero_halves); ++i)
        store_half(b + zero_halves[i], 0);
    b[12] = 1;
    b[31] = 0xff;
    b[32] = 0;
    b[34] = 0;
    b[35] = 1;
    store_half(b + 26, 1);
    store_half(b + 40, 4096);
    store_half(b + 42, 4096);
    store_half(b + 44, 36);
    store_half(b + 46, 3);
    memset(b + 64, 0, 4);
}

/* The external service is an observation point, with synthetic outputs only. */
void func_80059888(TutorialMessageRect *rect, int arg1, int arg2, int arg3)
{
    unsigned char *b = slot_bytes(&actual);
    unsigned char *want = slot_bytes(&expected);
    size_t i;

    check(++rect_calls == 1, "duplicate rectangle callback");
    check(rect != NULL, "missing rectangle");
    check(arg1 == 0 && arg2 == 0 && arg3 == 0, "rectangle scalar arguments");
    check(rect->x == expected_x && rect->y == expected_y, "rectangle coordinates");
    check(rect->width == 56 && rect->height == 48, "rectangle dimensions");
    check_memory("at rectangle callback");

    /* A writable rectangle copy must be independent of slot and clear storage. */
    rect->x ^= 0xffffu;
    rect->y ^= 0xffffu;
    rect->width = 0x8181;
    rect->height = 0x7eee;
    check_memory("after changing callback rectangle");

    /* A later clear or slot store can now be distinguished from an earlier one. */
    b[12] = want[12] = 0x6e;
    b[13] = want[13] = 0x5b;
    b[14] = want[14] = 0x93;
    store_half(b + 74, 0xbc8d);
    store_half(want + 74, 0xbc8d);
    for (i = 0; i < CLEAR_WORDS; ++i) {
        unsigned short changed = (unsigned short)(0xfffeu - i * 17u);
        expected_words[i] = D_80127C0C[i] = changed;
    }
}

static void check_direct_clear(unsigned short x, unsigned short y)
{
    operation = "clear";
    prepare(x, y);
    memset(expected_words, 0, sizeof(expected_words));
    func_80139DC8();
    check_memory("after direct clear");
    func_80139DC8();
    check_memory("after repeated clear");
    check(rect_calls == 0, "clear invoked rectangle callback");
}

static void check_direct_rect(unsigned short x, unsigned short y)
{
    operation = "rectangle";
    prepare(x, y);
    func_80139634(&actual.slot);
    check(rect_calls == 1, "rectangle callback count");
    check_memory("after rectangle returns");
}

static void check_defaults(unsigned short x, unsigned short y)
{
    operation = "defaults";
    prepare(x, y);
    expect_defaults_before_rect();
    func_80137840(&actual.slot);
    check(rect_calls == 1, "defaults rectangle callback count");
    slot_bytes(&expected)[13] = 0;
    memset(expected_words, 0, sizeof(expected_words));
    check_memory("after defaults return");
}

int main(void)
{
    static const unsigned short coordinates[][2] = {
        {0x0000, 0xffff}, {0xffff, 0x0000}, {0x8000, 0x7fff},
        {0x7fff, 0x8000}, {0x00ff, 0xff00}, {0xf135, 0x9a6c}
    };
    for (trial = 0; trial < sizeof(coordinates) / sizeof(coordinates[0]); ++trial) {
        unsigned short x = coordinates[trial][0];
        unsigned short y = coordinates[trial][1];
        check_direct_clear(x, y);
        check_direct_rect(x, y);
        check_defaults(x, y);
    }
    printf("%u cases per routine, %d clear halfwords per comparison\n", trial, CLEAR_WORDS);
    return EXIT_SUCCESS;
}
