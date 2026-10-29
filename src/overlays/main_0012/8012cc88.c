/* Owner re-derivation from retail assembly (clean input set). */
/* Moves an object by its velocity, then asks func_8012CEB0 about the swept position (+offset d); on a 0x6000 hit it zeroes the +0x14 word. Returns the query flags. */
#include "psx_types.h"
int func_8012CEB0(u16 *v, u16 *w, int mode);
int func_8012CC88(char *o, int mode, u16 *d) {
    u16 v[4];
    u16 w[4];
    int r;
    v[0] = *(u16 *)(o + 0x3A);
    v[1] = *(u16 *)(o + 0x3E);
    v[2] = *(u16 *)(o + 0x42);
    v[0] += d[0];
    v[1] += d[1];
    v[2] += d[2];
    *(int *)(o + 0x10) += *(int *)(o + 0x44);
    *(int *)(o + 0x14) += *(int *)(o + 0x48);
    *(int *)(o + 0x18) += *(int *)(o + 0x4C);
    *(int *)(o + 4) += *(int *)(o + 0x10);
    *(int *)(o + 8) += *(int *)(o + 0x14);
    *(int *)(o + 0xC) += *(int *)(o + 0x18);
    w[0] = *(u16 *)(o + 6);
    w[1] = *(u16 *)(o + 0xA);
    w[2] = *(u16 *)(o + 0xE);
    w[0] += d[0];
    w[1] += d[1];
    w[2] += d[2];
    r = func_8012CEB0(v, w, mode);
    w[0] -= d[0];
    w[1] -= d[1];
    w[2] -= d[2];
    *(u16 *)(o + 6) = w[0];
    *(u16 *)(o + 0xA) = w[1];
    *(u16 *)(o + 0xE) = w[2];
    if ((r & 0x6000) != 0) {
        *(int *)(o + 0x14) = 0;
    }
    return r;
}
