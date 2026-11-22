#include "mine_script_queue.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

u16 D_801A07D8[512];
s16 D_801A0BDC;
const void *D_801A0BFC;

static unsigned width(unsigned opcode)
{
    switch (opcode) {
    case 1: case 5: return 4;
    case 19: return 3;
    case 2: case 3: case 4: case 6: case 18: case 20: case 31: return 2;
    case 23: case 28: case 29: case 32: return 5;
    default: return 1;
    }
}

int main(void)
{
    u16 expected[512] = {0};
    unsigned index = 511;
    unsigned i, j;
    unsigned char first[28] = {0}, latest[28] = {0};
    D_801A0BDC = 511;
    for (i = 0; i < 1100; ++i) {
        u16 command[5] = {(u16)(i % 39u), (u16)i, (u16)(i + 4u), 0xff00, 0x1234};
        if (i == 17) command[0] = 0xffff;
        func_80179B74(command);
        for (j = 0; j < width(command[0]); ++j) {
            expected[index] = command[j];
            index = (index + 1u) & 511u;
        }
        assert(D_801A0BDC == (s16)index);
        assert(!memcmp(expected, D_801A07D8, sizeof expected));
    }
    func_8017A094(first);
    assert(D_801A0BFC == first && D_801A07D8[index] == 0x15);
    func_8017A094(latest);
    assert(D_801A0BFC == latest && D_801A07D8[(index + 1u) & 511u] == 0x15);
    latest[4] = 0x7c;
    assert(((const unsigned char *)D_801A0BFC)[4] == 0x7c);
    assert(sizeof D_801A0BFC == sizeof(uintptr_t));
    memcpy(expected, D_801A07D8, sizeof expected);
    D_801A0BDC = -1;
    func_8017A094(first);
    assert(D_801A0BFC == latest && D_801A0BDC == -1);
    func_80179B74(NULL);
    assert(!memcmp(expected, D_801A07D8, sizeof expected));
    return 0;
}
