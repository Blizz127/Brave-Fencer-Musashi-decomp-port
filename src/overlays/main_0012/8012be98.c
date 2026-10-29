/* Owner re-derivation from retail assembly (clean input set). */
/* Builds two 3-component 16-bit vectors from two records and passes them to func_800132BC and returns its result. */
#include "psx_types.h"
int func_800132BC(u16 *a, u16 *b);
int func_8012BE98(u16 *a, u16 *b) {
    u16 v[3];
    u16 w[3];
    v[0] = a[3];
    v[1] = 0;
    v[2] = a[7];
    w[0] = b[0];
    w[1] = 0;
    w[2] = b[2];
    return func_800132BC(v, w);
}
