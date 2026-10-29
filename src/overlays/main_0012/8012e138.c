/* Owner re-derivation from retail assembly (clean input set). */
/* Samples the global 3-short position D_80126B5E.., transforms it into the object's node space, syncs the node (position + offsets, dirty flag), then runs func_80135888 and, if it hits, writes the position back. */
#include "psx_types.h"
extern u16 D_80126B5E;
extern u16 D_80126B62;
extern u16 D_80126B66;
void func_8012F038(int node, u16 *in, u16 *out);
void func_80049CAC(int a, int b);
void func_8012F14C(int node, u16 *in, u16 *out);
int func_80135888(int a, int b, u16 *c, u16 *d);
void func_8012E138(char *o) {
    u16 pos[3];
    u16 w2[3];
    u16 w[3];
    pos[0] = D_80126B5E;
    pos[1] = D_80126B62;
    pos[2] = D_80126B66;
    func_8012F038(*(int *)(o + 0x20) + 0x34, pos, w);
    if (*(int *)(o + 0x20) != 0) {
        int n = *(int *)(o + 0x20);
        char *q;
        u16 x;
        func_80049CAC(n + 0x10, n + 0x34);
        q = (char *)*(int *)(o + 0x20);
        x = *(u16 *)(o + 6) + *(u16 *)(o + 0x50);
        *(u16 *)(q + 8) = x;
        *(int *)(q + 0x48) = (short)x;
        x = *(u16 *)(o + 0xA) + *(u16 *)(o + 0x52);
        *(u16 *)(q + 0xA) = x;
        *(int *)(q + 0x4C) = (short)x;
        x = *(u16 *)(o + 0xE) + *(u16 *)(o + 0x54);
        *(u16 *)(q + 0xC) = x;
        *(u16 *)(q + 0x2C) |= 1;
        *(int *)(q + 0x50) = (short)x;
    }
    func_8012F14C(*(int *)(o + 0x20) + 0x34, w, w2);
    if (func_80135888(*(int *)(o + 0x20), *(int *)(o + 0x58), w2, pos) != 0) {
        D_80126B5E = pos[0];
        D_80126B62 = pos[1];
        D_80126B66 = pos[2];
    }
}
