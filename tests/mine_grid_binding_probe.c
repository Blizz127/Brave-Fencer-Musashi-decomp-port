/* Observe the room binding when it calls the real offset fixup. */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "psx_types.h"

#define func_80136BC4 relocate_floor_offsets
#include "../src/overlays/sc02_0039/80136bc4.c"
#undef func_80136BC4

u32 D_8019F9B8, D_8019F9BC, D_8019F9C0;
void func_801336E8(void *primary, u32 second, u32 third);
static void *expected_record;
static u32 previous_second, previous_third;
static unsigned calls;

void func_80136BC4(void *primary)
{
    assert(primary == expected_record);
    assert(D_8019F9B8 == (u32)(uintptr_t)primary);
    assert(D_8019F9BC == previous_second && D_8019F9C0 == previous_third);
    ++calls;
    relocate_floor_offsets(primary);
}

static void check(u32 marker)
{
    u32 expected[] = {71, 16, marker, 0, UINT32_MAX, 0x80000000u, 8, 0x7fffffffu, 99, 371};
    void *record = malloc(sizeof expected);
    assert(record);
    expected_record = record;
    memcpy(record, expected, sizeof expected);
    for (unsigned i = 0; i < 3; ++i) {
        u32 second = 7u - 5u*i, third = 0xfff36901u*i;
        previous_second = D_8019F9BC;
        previous_third = D_8019F9C0;
        unsigned before = calls;
        if (expected[2] == 32) {
            for (unsigned j = 2; j < 8; ++j)
                expected[j] += (u32)(uintptr_t)record;
        }
        func_801336E8(record, second, third);
        assert(calls == before + 1 && !memcmp(record, expected, sizeof expected));
        assert(D_8019F9B8 == (u32)(uintptr_t)record);
        assert(D_8019F9BC == second && D_8019F9C0 == third);
    }
    free(record);
}

int main(void)
{
    D_8019F9B8 = 0x92345678u;
    D_8019F9BC = 2;
    D_8019F9C0 = 3;
    func_801336E8(NULL, UINT32_MAX, 0);
    assert(calls == 0 && D_8019F9B8 == 0x92345678u);
    assert(D_8019F9BC == UINT32_MAX && D_8019F9C0 == 0);
    check(32);
    check(31);
    check(33);
    check(UINT32_MAX);
    check(0);
    return 0;
}
