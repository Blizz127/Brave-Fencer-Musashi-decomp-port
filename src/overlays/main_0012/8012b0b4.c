#include "psx_types.h"

typedef struct {
    s32 x : 16;
    s32 y : 16;
} B2;

extern s32 func_80047948(s32 a);
extern s32 func_8004787C(s32 a);

B2 *func_8012B0B4(B2 *out, s32 ang, s32 r) {
    B2 t;
    s32 a;
    a = (ang - 0x400) & 0xFFF;
    t.x = (func_80047948(a) * r) >> 12;
    t.y = (func_8004787C(a) * -r) >> 12;
    *out = t;
    return out;
}
