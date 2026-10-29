/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_80156648(void *o, u32 a1);
extern void func_80147078(void *o, u16 a1);
extern void func_80157D74(void *o);
extern u16 D_80186FAC[];

void func_80157D20(void *o, u32 a1) {
    func_80156648(o, a1);
    func_80147078(o, D_80186FAC[a1]);
    func_80157D74(o);
}
