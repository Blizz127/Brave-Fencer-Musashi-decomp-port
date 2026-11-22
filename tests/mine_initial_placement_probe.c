#include "mine_initial_placement.h"
#include "scene_object_probes.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

Obj8014A2E4 D_801202A0[96];
MineInitialPlacement *D_801274C8, *D_801274CC;
static MineInitialPlacement primary[5], secondary[4], replacement[4];
static MineInitialPlacement *expected;
static unsigned visits, mutate, last_slot;
static s32 reply;

s32 func_8012C890(void *entry, u8 *actor, u8 *owner)
{
    MineInitialPlacement *row = entry;
    assert(row == expected && owner == 0);
    assert(row->flags & 0x8000);
    Obj8014A2E4 *slot = (Obj8014A2E4 *)actor;
    assert(slot->active == 0);
    last_slot = (unsigned)(slot - D_801202A0);
    slot->active = row->type;
    ++visits;
    if (mutate == 1 && visits == 1) {
        D_801274C8 = 0;
        D_801274CC = replacement;
        expected = row + 1;
    }
    if (mutate == 2)
        row[1].type = 0;
    if (mutate == 3) {
        row->flags &= 0x7fff;
        slot->active = 0;
    }
    return reply;
}

static void test_allocator(void)
{
    static MineInitialPlacement row = {{0}, 23, -64, 0, 531, 4, 17};
    expected = &row;
    for (unsigned direction = 0; direction < 2; ++direction) {
        for (unsigned free_slot = 0; free_slot <= 96; ++free_slot) {
            for (unsigned i = 0; i != 96; ++i)
                D_801202A0[i].active = i == free_slot ? 0 : 16;
            u16 initial = (u16)(0x4201 | (direction << 11));
            row.flags = initial;
            visits = mutate = 0;
            reply = (s32)(-1000 - (int)free_slot);
            s32 result = func_8012C750(&row);
            if (free_slot == 96) {
                assert(result == 0 && visits == 0 && row.flags == initial);
            } else {
                assert(result == reply && visits == 1 && last_slot == free_slot);
                assert(row.flags == (initial | 0x8000));
            }
        }
        memset(D_801202A0, 0, sizeof D_801202A0);
        row.flags = (u16)(direction << 11);
        reply = 0;
        mutate = 3;
        visits = 0;
        assert(func_8012C750(&row) == 0 && visits == 1);
        assert(last_slot == (direction ? 95u : 0u));
        assert(row.flags == (direction << 11) && D_801202A0[last_slot].active == 0);
    }
}

static void test_live_lists(void)
{
    memset(D_801202A0, 0, sizeof D_801202A0);
    memset(primary, 0, sizeof primary);
    memset(secondary, 0, sizeof secondary);
    memset(replacement, 0, sizeof replacement);
    primary[0].type = 7;
    primary[1].type = 8;
    primary[1].flags = 0x802;
    secondary[0].type = replacement[0].type = replacement[1].type = 7;
    secondary[0].flags = replacement[0].flags = replacement[1].flags = 0xf123;
    replacement[2].flags = 0xfe14;
    replacement[3].type = 4;
    replacement[3].flags = 0xf701;
    D_801274C8 = primary;
    D_801274CC = secondary;
    expected = primary;
    visits = 0;
    mutate = 1;
    func_8013E4B4();
    assert(visits == 2 && last_slot == 95 && D_801202A0[0].active == 7);
    assert(primary[0].flags == 0x8000 && primary[1].flags == 0x8802);
    assert(secondary[0].flags == 0xf123 && replacement[0].flags == 0x7123);
    assert(replacement[1].flags == 0x7123 && replacement[2].flags == 0xfe14);
    assert(replacement[3].flags == 0xf701);
    D_801274C8 = primary;
    D_801274CC = primary;
    visits = 0;
    mutate = 2;
    expected = primary;
    func_8013E4B4();
    assert(visits == 1 && primary[0].flags == 0 && primary[1].flags == 0x8802);
    D_801274C8 = D_801274CC = 0;
    func_8013E4B4();
    assert(visits == 1);
}

int main(void)
{
    _Static_assert(sizeof(MineInitialPlacement) == 20, "placement stride");
    _Static_assert(sizeof(Obj8014A2E4) == 268, "actor stride");
    test_allocator();
    test_live_lists();
    return 0;
}
