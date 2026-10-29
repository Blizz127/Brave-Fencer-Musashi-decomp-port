/* Owner re-derivation from retail assembly (clean input set). */
/* Sibling of func_80015018 for the second field set (0x46/0x47 and 0x36/0x37) of a
 * 76-byte record; gated on the u16 at 0x3A and the kind byte 'S'/'s'. */
#include "psx_types.h"

typedef struct {
    u8 kind;
    u8 pad01[0x35];
    u8 b36;
    u8 b37;
    u8 pad38[2];
    u16 w3A;
    u8 pad3C[0xA];
    u8 b46;
    u8 b47;
    u8 pad48[4];
} InputRec;

extern InputRec D_80078D98[];
extern void func_80015118(u16 *arg0);

u16 func_80014ED4(u8 idx) {
    InputRec *r = &D_80078D98[idx];
    struct { u16 w[4]; } a;
    u32 v;
    s32 k;
    InputRec *q;

    if (r->w3A != 0) {
        return r->w3A;
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
    a.w[0] = q->b47;
    a.w[1] = q->b46;
    a.w[2] = q->b37;
    a.w[3] = q->b36;
    func_80015118(&a.w[0]);
    func_80015118(&a.w[1]);
    func_80015118(&a.w[2]);
    func_80015118(&a.w[3]);
    v = 0;
    if (a.w[0] == 0x80) {
        if (a.w[2] != 0x80) {
            v = 0x1000;
            if (a.w[2] >= 0x80) {
                v = 0x4000;
            }
        }
    }
    if (a.w[1] == 0x80) {
        if (a.w[3] != 0x80) {
            v |= (a.w[3] < 0x80) ? 0x8000 : 0x2000;
        }
    }
    return v;
}
