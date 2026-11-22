#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "mine_frame_slots.h"

_Static_assert(sizeof(MineFrameSlot) == 32, "slot stride");
MineFrameSlot D_80115218[4];
void (*D_801824C8[256])(MineFrameSlot *);

static unsigned visits[4], handlers[4], count, mutate;

static void record(MineFrameSlot *slot, unsigned handler)
{
    ptrdiff_t index = slot - D_80115218;
    assert(index >= 0 && index < 4 && count < 4);
    visits[count] = (unsigned)index;
    handlers[count++] = handler;
    ++slot->reserved[0];
}

static void secondary(MineFrameSlot *slot)
{
    record(slot, 1);
}

static void primary(MineFrameSlot *slot)
{
    record(slot, 0);
    if (mutate && slot == D_80115218) {
        D_80115218[1].active = 0;
        D_80115218[2].active = 4;
        D_80115218[2].handler = 255;
        D_801824C8[0] = secondary;
    }
}

int main(void)
{
    unsigned mask, i, n;
    D_801824C8[0] = primary;
    D_801824C8[1] = secondary;
    D_801824C8[255] = secondary;
    for (mask = 0; mask < 16; ++mask) {
        memset(D_80115218, 0, sizeof(D_80115218));
        for (i = 0; i < 4; ++i) {
            D_80115218[i].active = mask & (1u << i);
            D_80115218[i].handler = i & 1;
        }
        count = 0;
        func_8016E95C();
        n = 0;
        for (i = 0; i < 4; ++i) {
            unsigned active = (mask >> i) & 1;
            assert(D_80115218[i].reserved[0] == active);
            if (active) {
                assert(visits[n] == i && handlers[n] == (i & 1));
                ++n;
            }
        }
        assert(count == n);
    }

    memset(D_80115218, 0, sizeof(D_80115218));
    D_80115218[0].active = 1;
    D_80115218[1].active = 255;
    D_80115218[3].active = 1;
    count = 0;
    mutate = 1;
    func_8016E95C();
    assert(count == 3);
    assert(visits[0] == 0 && handlers[0] == 0);
    assert(visits[1] == 2 && handlers[1] == 1);
    assert(visits[2] == 3 && handlers[2] == 1);
    assert(D_80115218[1].reserved[0] == 0);
    return 0;
}
