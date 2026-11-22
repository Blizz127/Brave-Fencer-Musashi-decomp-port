#include "actor_position_updates.h"
#include "mine_initial_placement.h"
#include <assert.h>
#include <string.h>

Obj8014A2E4 D_801202A0[97];
s16 D_801270C0, D_801270C2;
u16 D_801270C4[2];
u8 D_801270C6, D_801270C7;
s8 D_801270C8[1028];
s32 D_801274D8, D_801274DC, D_801274E0;
static unsigned stage, fill;
void func_80178608(void);

static void check_bytes(const void *storage, unsigned count, unsigned value)
{
    const unsigned char *bytes = storage;
    for (unsigned i = 0; i < count; ++i)
        assert(bytes[i] == value);
}

static void check_field(void)
{
    assert(D_801270C0 == 1 && D_801270C2 == 0 && D_801270C4[0] == 1);
    assert(D_801270C6 == 0 && D_801270C7 == 0);
    assert(D_801274D8 == 0x240000 && D_801274DC == 0x271000 && D_801274E0 == 0x400000);
}

void func_80016714(s8 *destination, s32 size)
{
    if (stage == 0) {
        assert(destination == (s8 *)D_801202A0 && size == 96 * 268);
        assert(D_801270C0 == -13 && D_801270C4[0] == 1317);
        assert(D_801274D8 == -981 && D_801274DC == -333 && D_801274E0 == 2288);
        check_bytes(destination, size, fill);
    } else {
        assert(stage == 1 && destination == D_801270C8 && size == 1024);
        check_field();
        check_bytes(D_801202A0, 96 * 268, 0);
        check_bytes(destination, size, fill);
    }
    memset(destination, 0, size);
    ++stage;
}

void func_8013E4B4(void)
{
    assert(stage == 2);
    check_field();
    check_bytes(D_801202A0, 96 * 268, 0);
    check_bytes(D_801270C8, 1024, 0);
    D_801202A0[95].active = 79;
    D_801270C8[1023] = 92;
    D_801270C4[0] = 73;
    ++stage;
}

int main(void)
{
    _Static_assert(sizeof(Obj8014A2E4) == 268, "actor stride");
    for (unsigned i = 0; i < 8; ++i) {
        fill = (i * 67) & 255;
        stage = 0;
        D_801270C0 = -13;
        D_801270C2 = -103;
        D_801270C4[0] = 1317;
        D_801270C4[1] = 0x18fa;
        D_801270C6 = 0x63;
        D_801270C7 = 0xff;
        D_801274D8 = -981;
        D_801274DC = -333;
        D_801274E0 = 2288;
        memset(D_801202A0, fill, sizeof(D_801202A0));
        memset(D_801270C8, fill, sizeof(D_801270C8));
        func_80178608();
        assert(stage == 3 && D_801202A0[95].active == 79);
        assert(D_801270C8[1023] == 92 && D_801270C4[0] == 73);
        assert(D_801270C4[1] == 0x18fa);
        check_bytes(&D_801202A0[96], sizeof(D_801202A0[96]), fill);
        check_bytes(&D_801270C8[1024], 4, fill);
    }
    return 0;
}
