/* Owner re-derivation from retail assembly (clean input set). */
/* Counts down the fade timer at +0xAA and applies a scale (0x700..0x1000, ramping at both ends) via func_80178744. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x5E];
    u16 mode;   /* 0x5E */
    u8 pad60[0x4A];
    s16 timer;  /* 0xAA */
} Obj8017869C;

extern void func_80178744(Obj8017869C *obj, s32 scale);

void func_8017869C(Obj8017869C *obj)
{
    s32 t;
    s32 scale;

    if (obj->timer == 0) {
        return;
    }
    t = --obj->timer;
    if (t == 0) {
        scale = 0x1000;
        if (obj->mode == 0x1E) {
            obj->mode = 0;
        }
    } else if (t > 0xF0) {
        scale = 0x1000 - (0x100 - t) * 0x90;
    } else if (t < 0x10) {
        scale = 0x1000 - t * 0x90;
    } else {
        scale = 0x700;
    }
    func_80178744(obj, scale);
}
