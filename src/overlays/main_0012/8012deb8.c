/* Owner re-derivation from retail assembly (clean input set). */
/* Computes two vectors via func_8012F214, tests them with func_8012DF34 and on success latches the 16-bit value at +0x12 of the node at +0x20; returns whether the test passed. */
#include "psx_types.h"
extern u16 D_80126B9A;
void func_8012F214(char *o, int a1, int *out);
int func_8012DF34(char *o, int *a, int *b);
int func_8012DEB8(char *o, int a1, int a2) {
    int v[2];
    int w[2];
    func_8012F214(o, a1, v);
    func_8012F214(o, a2, w);
    if (func_8012DF34(o, v, w) != 0) {
        D_80126B9A = *(u16 *)(*(int *)(o + 0x20) + 0x12);
        return 1;
    }
    return 0;
}
