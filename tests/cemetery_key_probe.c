#include <assert.h>
#include <string.h>
#include "event_flags.h"

enum { FIRST = 47, END = 59 };
u8 D_800BA1B8[256];
u8 D_800AE648[64];
s16 D_80078EAC;

void func_8017D828(void *script);
u32 func_800D0EC4(void);
void func_800D0F0C(u32 slot, u32 item);
void func_800291A0(s32 index, s32 value);

static unsigned stage, free_slot;
static int story;
static char script_context;

u32 probe_free(void)
{
    assert(stage++ == 0);
    return func_800D0EC4();
}

void probe_item(u32 slot, u32 item)
{
    assert(stage++ == 1 && slot == free_slot && item == 77);
    assert(D_800BA1B8[204] == 14 && story == 50);
    /* The script normally prompts a full inventory before this callback.
       For a direct call with no free slot, record zero without writing before the array. */
    if (slot)
        func_800D0F0C(slot, item);
}

void probe_variable(s32 index, s32 value)
{
    assert(stage++ == 2 && index == 204 && value == 15);
    if (free_slot) {
        unsigned stamp = 2 * free_slot - 74;
        assert(D_800BA1B8[free_slot] == 77);
        assert(D_800BA1B8[stamp] == (u8)D_80078EAC);
        assert(D_800BA1B8[stamp + 1] == (u16)D_80078EAC >> 8);
    }
    func_800291A0(index, value);
}

void func_80029514(int progress)
{
    assert(stage++ == 3 && progress == 60 && D_800BA1B8[204] == 15);
    story = progress;
}

u32 probe_flag(u32 bit, u32 set)
{
    assert(stage++ == 4 && story == 60 && bit == 230 && set == 1);
    return func_80029124(bit, set);
}

void func_80171928(void *context)
{
    assert(stage++ == 5 && context == &script_context);
    assert(D_800AE648[28] == 0x6b && story == 60);
}

int main(void)
{
    unsigned mask, i;
    for (mask = 0; mask < (1u << (END - FIRST)); ++mask) {
        u8 expected[sizeof D_800BA1B8];
        memset(D_800BA1B8, 0x9a, sizeof D_800BA1B8);
        memset(D_800AE648, 0x2b, sizeof D_800AE648);
        D_800BA1B8[204] = 14;
        free_slot = 0;
        for (i = 0; i < END - FIRST; ++i) {
            D_800BA1B8[FIRST + i] = mask & (1u << i) ? (u8)(1 + i) : 0;
            if (!free_slot && !D_800BA1B8[FIRST + i])
                free_slot = FIRST + i;
        }
        D_80078EAC = (s16)((int)mask - 2048);
        memcpy(expected, D_800BA1B8, sizeof expected);
        if (free_slot) {
            unsigned stamp = 2 * free_slot - 74;
            expected[free_slot] = 77;
            expected[stamp] = (u8)D_80078EAC;
            expected[stamp + 1] = (u8)((u16)D_80078EAC >> 8);
        }
        expected[204] = 15;
        stage = 0;
        story = 50;
        func_8017D828(&script_context);
        assert(stage == 6 && !memcmp(expected, D_800BA1B8, sizeof expected));
        for (i = 0; i < sizeof D_800AE648; ++i)
            assert(D_800AE648[i] == (i == 28 ? 0x6b : 0x2b));
    }
    return 0;
}
