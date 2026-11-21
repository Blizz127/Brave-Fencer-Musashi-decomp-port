#include <assert.h>
#include <stdint.h>

#include "psx_types.h"

s32 func_8017C24C(void);
s32 func_8017C348(void);

#ifdef D_80115112
/* The guarded build binds the shared counter to the middle array element.
 * A C array guarantees adjacency on every host. */
static u16 cells[3];
u16 *menu_counter = &cells[1];
#else
u16 D_80115112;
#endif

static void check_increment(s32 (*advance)(void), u32 initial)
{
    s32 expected = (s32)initial + 1;
#ifdef D_80115112
    u16 left = (u16)(initial ^ 0x5aa5u);
    u16 right = (u16)~left;
    cells[0] = left;
    cells[2] = right;
#endif

    D_80115112 = (u16)initial;
    assert(advance() == expected);
    assert(D_80115112 == (u16)expected);

#ifdef D_80115112
    assert(cells[0] == left);
    assert(cells[2] == right);
#endif
}

int main(void)
{
    u32 initial;
    for (initial = 0; initial <= UINT16_MAX; ++initial) {
        check_increment(func_8017C24C, initial);
        check_increment(func_8017C348, initial);
    }

    /* The second callback must observe the first one's store. */
    D_80115112 = UINT16_MAX - 1;
    assert(func_8017C24C() == UINT16_MAX);
    assert(func_8017C348() == (s32)UINT16_MAX + 1);
    assert(D_80115112 == 0);
    assert(func_8017C24C() == 1);
    assert(D_80115112 == 1);
    return 0;
}
