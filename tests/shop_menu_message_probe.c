#include <assert.h>
#include <limits.h>
#include <string.h>

#include "psx_types.h"

enum { ROW_WORDS = 10, TABLE_WORDS = 3 * ROW_WORDS };
enum { PURCHASE_SUCCESS = 1, INVENTORY_FULL = 2, FUNDS_SHORT = 3 };

_Static_assert(CHAR_BIT == 8 && sizeof(s32) == 4 && sizeof(u32) == 4,
               "message words occupy four bytes");

s16 D_80115126;
u32 D_801C16C4[TABLE_WORDS];
static u32 initial_table[TABLE_WORDS];
static s32 latch_words[3];
s32 *shop_test_latch = &latch_words[1];

u32 func_8018C89C(s16 selector);
void func_8018C8D4(s16 selector);

static void check(s16 page, s16 selector, unsigned slot)
{
    u32 value = initial_table[slot];
    u32 expected_latch[3] = {0x12345678u, ~value, 0x6a7b8c9du};

    D_80115126 = page;
    memcpy(latch_words, expected_latch, sizeof(latch_words));
    assert(func_8018C89C(selector) == value);
    assert(D_80115126 == page);
    assert(memcmp(D_801C16C4, initial_table, sizeof(initial_table)) == 0);
    assert(memcmp(latch_words, expected_latch, sizeof(latch_words)) == 0);

    func_8018C8D4(selector);
    expected_latch[1] = value;
    assert(memcmp(latch_words, expected_latch, sizeof(latch_words)) == 0);
    assert(D_80115126 == page);
    assert(memcmp(D_801C16C4, initial_table, sizeof(initial_table)) == 0);
}

int main(void)
{
    for (unsigned i = 0; i < TABLE_WORDS; ++i)
        initial_table[i] = 0x81000001u + 0x00030031u * i;
    initial_table[PURCHASE_SUCCESS] = 0u;
    initial_table[INVENTORY_FULL] = 0x80000000u;
    initial_table[FUNDS_SHORT] = 0xffffffffu;
    initial_table[ROW_WORDS + PURCHASE_SUCCESS] = 0x7fffffffu;
    memcpy(D_801C16C4, initial_table, sizeof(initial_table));

    for (unsigned i = 0; i < TABLE_WORDS; ++i)
        check(i / ROW_WORDS, i % ROW_WORDS, i);

    /* Synthetic signed selectors reach earlier slots inside this array.
     * No out-of-bounds table index or dereference of a payload is modeled. */
    check(1, -1, 9);
    check(2, -10, 10);
    check(2, -20, 0);
    return 0;
}
