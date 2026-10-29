/* Owner re-derivation from retail assembly (clean input set). */
/* Per-record (76-byte) input mapper: when the record's u16 at 0x32 is clear and its
 * kind byte is 'S' or 's', clamp two byte fields via func_80015118 and fold them into a flag mask. */
#include "psx_types.h"

typedef struct {
    u8 kind;
    u8 pad01[0x31];
    u16 w32;
    u8 pad34[2];
    u8 b36;
    u8 b37;
    u8 pad38[0x14];
} InputRec;

extern InputRec D_80078D98[];
extern void func_80015118(u16 *arg0);

u16 func_80015018(u8 idx) {
    InputRec *r = &D_80078D98[idx];
    struct { u16 x, y; } a;
    u32 v;
    s32 k;
    InputRec *q;

    if (r->w32 != 0) {
        return r->w32;
    }
    k = r->kind;
    if (k != 0x53) {
        if (k <= 0x53) {
            return 0;
        }
        if (k != 0x73) {
            return 0;
        }
    }
    q = &D_80078D98[idx];
    a.x = q->b37;
    a.y = q->b36;
    func_80015118(&a.x);
    func_80015118(&a.y);
    v = 0;
    if (a.x != 0x80) {
        v = 0x1000;
        if (a.x >= 0x80) {
            v = 0x4000;
        }
    }
    if (a.y != 0x80) {
        v |= (a.y < 0x80) ? 0x8000 : 0x2000;
    }
    return v;
}
