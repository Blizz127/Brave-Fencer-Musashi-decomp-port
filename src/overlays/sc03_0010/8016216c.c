#include "steamwood_timer.h"

extern void func_800291C8(int offset, short value);
extern unsigned int D_8019EDE8[3];

void func_8016216C(void)
{
    int i;

    func_800291C8(8, 0);
    for (i = 0; i < 3; ++i)
        func_80016714(D_8019EDE8[i], 0x38);
}
