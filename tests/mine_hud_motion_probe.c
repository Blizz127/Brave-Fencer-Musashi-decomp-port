#include <assert.h>
#include <string.h>
#include "mine_hud_motion.h"

MineFrameSlot D_80115218[4];
void (*D_801824C8[4])(MineFrameSlot *);
u16 D_801824BC[256];
u8 D_801824AC[4];

static u8 restore_counter, injected_counter, expected_shade;
static u16 expected_x;
static unsigned calls;
static MineFrameSlot observed;

void func_8016EC0C(MineFrameSlot *slot, s32 shade)
{
    assert(shade == expected_shade && mine_hud_x(slot) == expected_x);
    observed = *slot;
    ++calls;
    if (restore_counter) {
        slot->reserved[0] = injected_counter;
        slot->handler = 31;
    }
}

int main(void)
{
    MineFrameSlot *slot = &D_80115218[2];
    MineFrameSlot before;
    memset(D_80115218, 0x5b, sizeof(D_80115218));
    before = *slot;
    func_8016E918(2, 10);
    assert(memcmp(slot, &before, sizeof(*slot)) == 0);
    func_8016E918(2, -1);
    assert(slot->active == 3 && slot->handler == 0 && slot->reserved[1] == 255);
    assert(D_80115218[1].active == 0x5b && D_80115218[3].active == 0x5b);

    D_801824BC[5] = 13;
    slot->handler = 1;
    slot->reserved[0] = 5;
    mine_hud_set_x(slot, 65530);
    expected_x = 7;
    expected_shade = 128;
    func_8016EAC0(slot);
    assert(calls == 1 && observed.reserved[0] == 5 && observed.handler == 1);
    assert(slot->handler == 2 && slot->reserved[0] == 32);

    D_801824AC[0] = 135;
    expected_shade = 135;
    func_8016EB3C(slot);
    assert(calls == 2 && observed.reserved[0] == 32 && slot->reserved[0] == 31);
    restore_counter = 1;
    injected_counter = 0;
    D_801824AC[3] = 241;
    expected_shade = 241;
    func_8016EB3C(slot);
    assert(calls == 3 && observed.reserved[0] == 31 && slot->handler == 32 && slot->reserved[0] == 5);

    expected_shade = 128;
    expected_x = 65530;
    func_8016EBA8(slot);
    assert(calls == 4 && observed.active == 3 && observed.reserved[0] == 5);
    assert(slot->active == 0 && slot->reserved[0] == 255 && slot->handler == 31);

    injected_counter = 255;
    D_801824BC[255] = 20;
    expected_x = 14;
    func_8016EAC0(slot);
    assert(calls == 5 && slot->reserved[0] == 0 && slot->handler == 31);
    return 0;
}
