/* Owner re-derivation from retail assembly (clean input set). */
/* Projects the eight corners of the object's bounding box (the +0x58 pointer or the b argument) and draws its twelve edges. */
#include "psx_types.h"

extern void func_8012D3B4(void *a, void *b, u32 color);
extern void func_8012F214(void *o, void *v, void *out);

void func_8012D098(u16 *o, u16 *b) {
    s16 c[8][4];
    s16 v[3];
    u32 w;
    if (o != 0 && o[0] != 0) {
        if (b == 0) {
            w = *(u32 *)((u8 *)o + 0x58);
            if (w == 0) {
                return;
            }
            b = (u16 *)((w & 0xFFFFFFF) | 0x80000000);
        }
        v[0] = b[2];
        v[1] = b[4];
        v[2] = b[6];
        func_8012F214(o, v, c[0]);
        v[0] = b[3];
        v[1] = b[4];
        v[2] = b[6];
        func_8012F214(o, v, c[1]);
        v[0] = b[2];
        v[1] = b[5];
        v[2] = b[6];
        func_8012F214(o, v, c[2]);
        v[0] = b[3];
        v[1] = b[5];
        v[2] = b[6];
        func_8012F214(o, v, c[3]);
        v[0] = b[2];
        v[1] = b[4];
        v[2] = b[7];
        func_8012F214(o, v, c[4]);
        v[0] = b[3];
        v[1] = b[4];
        v[2] = b[7];
        func_8012F214(o, v, c[5]);
        v[0] = b[2];
        v[1] = b[5];
        v[2] = b[7];
        func_8012F214(o, v, c[6]);
        v[0] = b[3];
        v[1] = b[5];
        v[2] = b[7];
        func_8012F214(o, v, c[7]);
        func_8012D3B4(c[0], c[1], 0xFFFFFF);
        func_8012D3B4(c[1], c[3], 0xFFFFFF);
        func_8012D3B4(c[3], c[2], 0xFFFFFF);
        func_8012D3B4(c[2], c[0], 0xFFFFFF);
        func_8012D3B4(c[4], c[5], 0xFFFFFF);
        func_8012D3B4(c[5], c[7], 0xFFFFFF);
        func_8012D3B4(c[7], c[6], 0xFFFFFF);
        func_8012D3B4(c[6], c[4], 0xFFFFFF);
        func_8012D3B4(c[0], c[4], 0xFFFFFF);
        func_8012D3B4(c[1], c[5], 0xFFFFFF);
        func_8012D3B4(c[2], c[6], 0xFFFFFF);
        func_8012D3B4(c[3], c[7], 0xFFFFFF);
    }
}
