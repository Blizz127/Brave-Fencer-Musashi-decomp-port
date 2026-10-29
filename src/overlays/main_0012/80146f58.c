/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s16 func_8012A68C(void);
extern s16 func_8012A758(void);
extern void func_8004978C(s16 *a, s32 *b);
extern void func_80048FBC(s32 *m, void *a, void *b);

void func_80146F58(void *a, void *b) {
    s16 v[3];
    s32 m[8];
    v[0] = func_8012A68C();
    v[1] = func_8012A758();
    v[2] = 0;
    func_8004978C(v, m);
    func_80048FBC(m, a, b);
}
