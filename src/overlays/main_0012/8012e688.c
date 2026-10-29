/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct { s16 x, y, z; } V8012E688;
extern u8 D_800AF648;
extern void func_8004914C(void *p);
extern void func_800491AC(void *p);
extern void func_8004955C(void *a, void *b, void *c, void *d);
extern void func_8002D4C8(u16 command, u16 argument);

static inline void w914C(void *p) { func_8004914C(p); }
static inline void w91AC(void *p) { func_800491AC(p); }

void func_8012E688(u8 *o, s32 a, s32 b)
{
    V8012E688 v;
    s32 p;
    s32 u;
    s32 z;
    s32 pad;

    if (*(u8 **)(o + 0x20) != 0) {
        v.x = *(s32 *)(*(u8 **)(o + 0x20) + 0x48);
        v.y = *(s32 *)(*(u8 **)(o + 0x20) + 0x4C);
        v.z = *(s32 *)(*(u8 **)(o + 0x20) + 0x50);
        w914C(&D_800AF648);
        w91AC(&D_800AF648);
        func_8004955C(&v, &p, &u, &z);
        if (z < 0) {
            return;
        }
        if ((u16)(*(u16 *)&p + 0xC7) >= 0x18F) {
            return;
        }
        if ((u16)(*((u16 *)&p + 1) + 0xA9) >= 0x153) {
            return;
        }
    }
    func_8002D4C8(a, b);
}
