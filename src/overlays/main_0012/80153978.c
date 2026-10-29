/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { u16 x, y, z; u8 f; } T153978;
extern int func_80133784(s32 a, u16 *p, T153978 *v);
extern int func_801539F8(u8 *o, T153978 *v);

int func_80153978(u8 *o, u16 *p) {
    T153978 v;
    v.x = p[0];
    v.y = p[1] + 0x1F;
    v.z = p[2];
    if (func_80133784(0, p, &v) == 0x2000 && v.f == 0 && func_801539F8(o, &v) == 0) {
        return 0;
    }
    return 1;
}
