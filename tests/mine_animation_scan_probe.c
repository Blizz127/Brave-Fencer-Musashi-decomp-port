#include "mine_animation_frames.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

static Obj8014A2E4 actor;
static u32 base, expected_address, mapped, calls;
static s16 actions[32];
static int replace_key;

static void set_key(u32 key)
{
    u8 *p = (u8 *)&actor + 0x90;
    p[0] = (u8)key;
    p[1] = (u8)(key >> 8);
    p[2] = (u8)(key >> 16);
    p[3] = (u8)(key >> 24);
}

static void reset(u32 key, u32 count)
{
    memset(&actor, 0, sizeof(actor));
    set_key(key);
    base = key;
    expected_address = key + 4u;
    mapped = count;
    calls = 0;
    replace_key = 0;
}

int bfm_animation_frame_operation(u32 address, s16 *op)
{
    u32 index = calls++;
    assert(address == expected_address);
    assert(address - base == 4u + 8u * index);
    expected_address += 8;
    if (index >= mapped)
        return 0;
    if (replace_key)
        set_key(UINT32_C(0x71fbe460));
    *op = actions[index];
    return 1;
}

int main(void)
{
    int action;
    unsigned i, j;
    for (action = INT16_MIN; action <= INT16_MAX; action++) {
        int terminates = action >= -2 && action <= 0;
        reset(UINT32_C(0x95706042), 2);
        actions[0] = (s16)action;
        actions[1] = -1;
        replace_key = 1;
        assert(func_8012ACE0(&actor) == (terminates ? 0 : 1));
        assert(calls == (terminates ? 1u : 2u));
    }
    for (i = 0; i < 32; i++) {
        for (j = 0; j < i; j++)
            actions[j] = (s16)(j & 1 ? -50 : (int)j + 1);
        actions[i] = (s16)(-(int)(i % 3));
        reset(UINT32_C(0x80200100), i + 1);
        assert(func_8012ACE0(&actor) == (s32)i && calls == i + 1);
        reset(UINT32_C(0x80200100), i);
        assert(func_8012ACE0(&actor) == -1 && calls == i + 1);
    }
    reset(UINT32_C(0xfffffff2), 2);
    actions[0] = 3;
    actions[1] = -2;
    assert(func_8012ACE0(&actor) == 1 && calls == 2);
    reset(UINT32_C(0xfffffffa), 1);
    actions[0] = 17;
    assert(func_8012ACE0(&actor) == -1 && calls == 1);
    reset(UINT32_C(0xfffffffc), 1);
    assert(func_8012ACE0(&actor) == -1 && calls == 0);
    reset(UINT32_C(0x80004001), 1);
    assert(func_8012ACE0(&actor) == -1 && calls == 0);
    reset(0, 0);
    assert(func_8012ACE0(&actor) == -1 && calls == 0);
    assert(func_8012ACE0(0) == -1 && calls == 0);
    return 0;
}
