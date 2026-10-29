/* Owner re-derivation from retail assembly (clean input set). */
/* Tests the twelve edges of the object's +0x58 box against func_8012DEB8: mask bit 1 selects the four x-edges, bit 2 the y-edges, bit 4 the z-edges; returns 1 on the first hit. */
#include "psx_types.h"

extern s32 func_8012DEB8(void *o, void *v, void *w);

s32 func_8012D714(u8 *o, s32 m) {
    s16 v[3];
    s16 w[3];
    u16 *b = (u16 *)((*(u32 *)(o + 0x58) & 0xFFFFFFF) | 0x80000000);
    if (m & 1) {
        v[0] = b[2];
        v[1] = b[4];
        v[2] = b[6];
        w[0] = b[3];
        w[1] = b[4];
        w[2] = b[6];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
        v[0] = b[2];
        v[1] = b[5];
        v[2] = b[6];
        w[0] = b[3];
        w[1] = b[5];
        w[2] = b[6];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
        v[0] = b[2];
        v[1] = b[4];
        v[2] = b[7];
        w[0] = b[3];
        w[1] = b[4];
        w[2] = b[7];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
        v[0] = b[2];
        v[1] = b[5];
        v[2] = b[7];
        w[0] = b[3];
        w[1] = b[5];
        w[2] = b[7];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
    }
    if (m & 2) {
        v[0] = b[2];
        v[1] = b[4];
        v[2] = b[6];
        w[0] = b[2];
        w[1] = b[5];
        w[2] = b[6];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
        v[0] = b[2];
        v[1] = b[4];
        v[2] = b[7];
        w[0] = b[2];
        w[1] = b[5];
        w[2] = b[7];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
        v[0] = b[3];
        v[1] = b[4];
        v[2] = b[6];
        w[0] = b[3];
        w[1] = b[5];
        w[2] = b[6];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
        v[0] = b[3];
        v[1] = b[4];
        v[2] = b[7];
        w[0] = b[3];
        w[1] = b[5];
        w[2] = b[7];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
    }
    if (m & 4) {
        v[0] = b[2];
        v[1] = b[4];
        v[2] = b[6];
        w[0] = b[2];
        w[1] = b[4];
        w[2] = b[7];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
        v[0] = b[2];
        v[1] = b[5];
        v[2] = b[6];
        w[0] = b[2];
        w[1] = b[5];
        w[2] = b[7];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
        v[0] = b[3];
        v[1] = b[4];
        v[2] = b[6];
        w[0] = b[3];
        w[1] = b[4];
        w[2] = b[7];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
        v[0] = b[3];
        v[1] = b[5];
        v[2] = b[6];
        w[0] = b[3];
        w[1] = b[5];
        w[2] = b[7];
        if (func_8012DEB8(o, v, w) != 0) {
            return 1;
        }
    }
    return 0;
}
