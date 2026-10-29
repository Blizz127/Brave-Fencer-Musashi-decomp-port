/* func_801638A0 [801638A0,80163950) - clean-room owner re-derivation (nmG). */
#include "psx_types.h"

typedef struct {
    u8 b[8];
} Raw8_801638A0;

extern s16 D_801152AA;
extern void func_80015978(void *src, void *dst);
extern void func_80015954(void *src, void *dst);
extern s32 func_80135004(s32 a, void *p, void *q);

s32 func_801638A0(u8 *o, void *b)
{
    Raw8_801638A0 rec;
    u8 *pos;

    rec = *(Raw8_801638A0 *)(o + 0x44);
    pos = o + 4;
    func_80015978(pos, b);
    if (func_80135004(1, &rec, b) != 0) {
        func_80015954(b, pos);
        if (D_801152AA >= -0x578) {
            return 0x8000;
        }
        if (D_801152AA >= -0xBCB) {
            return 0x4000;
        }
        return 0x2000;
    }
    return 0;
}
