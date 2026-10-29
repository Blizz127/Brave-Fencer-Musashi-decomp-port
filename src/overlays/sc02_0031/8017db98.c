/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_8014706C(void *o);
extern void func_80154274(void *o, void *a1);
extern void func_80171990(void *o);
extern u8 D_80188808[];

void func_8017DB98(void *o) {
    func_8014706C(o);
    func_80154274(o, D_80188808);
    func_80171990(o);
}
