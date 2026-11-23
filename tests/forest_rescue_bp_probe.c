#include <assert.h>
#include <limits.h>
#include "bincho_burst.h"

u16 D_80078EB6;
u16 D_80078EB8;
u8 D_80126B58[1];
static unsigned sounds;

void func_8002D4C8(s32 effect, s32 flags)
{
    assert(effect == 0xbfe && flags == 0);
    assert(D_80078EB6 == D_80078EB8);
    ++sounds;
}

void func_8017CCCC(void);

int main(void)
{
    D_80078EB6 = 155;
    D_80078EB8 = 111;
    func_8017CCCC();
    assert(D_80078EB6 == 160 && D_80078EB8 == 160 && sounds == 1);

    D_80078EB6 = 1632;
    D_80078EB8 = 300;
    func_8017CCCC();
    assert(D_80078EB6 == 1634 && D_80078EB8 == 1634 && sounds == 2);

    /* The game's halfword addition wraps before either limit is applied. */
    D_80078EB6 = 65534;
    func_8014BCEC(D_80126B58, 5);
    assert(D_80078EB6 == 3);
    D_80078EB8 = 65535;
    func_8014BD24(D_80126B58, 2);
    assert(D_80078EB8 == 1);
    func_8014BD24(D_80126B58, INT_MAX);
    assert(D_80078EB8 == 0);
    func_8014BD24(D_80126B58, -1);
    assert(D_80078EB8 == 3);
    return 0;
}
