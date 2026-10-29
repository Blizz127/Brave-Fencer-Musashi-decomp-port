/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
/* Copies three 16-bit fields (+6, +0xA, +0xE) of a record into a local vector and passes it to func_8012D664. */
/* a1/a2 pass straight through to func_8012D664 (the retail words leave them
 * untouched before the jal), so they are parameters here. */
void func_8012D664(u16 *v, s32 a1, s32 a2);
void func_8012D624(u16 *p, s32 a1, s32 a2) {
    u16 v[3];
    v[0] = p[3];
    v[1] = p[5];
    v[2] = p[7];
    func_8012D664(v, a1, a2);
}
