/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_8012F038(u8 *a, u16 *b, u16 *c);

void func_80149450(u8 *a, u8 *v) {
    u16 out[3];
    u16 in[3];
    in[0] = *(u16 *)(v + 2);
    in[1] = *(u16 *)(v + 6);
    in[2] = *(u16 *)(v + 0xA);
    func_8012F038(*(u8 **)(a + 0x20) + 0x34, in, out);
    *(u16 *)(v + 2) = out[0];
    *(u16 *)(v + 6) = out[1];
    *(u16 *)(v + 0xA) = out[2];
}
