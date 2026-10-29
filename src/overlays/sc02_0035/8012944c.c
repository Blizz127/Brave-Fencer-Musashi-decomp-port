/* Owner re-derivation from retail assembly (clean input set). */
/* func_8012944C [8012944C,8012956C) - owner re-derivation from retail assembly (clean input set). * Walks the two 0x54-byte entries at D_800B9A78 and dispatches on each entry's state word. */
#include "psx_types.h"

typedef struct {
    u8 p0[0x1E];
    u16 f1E;
    u8 p20[0x14];
    u16 f34;
    u8 p36[4];
    u16 f3A;
    u8 p3C[6];
    u16 f42;
    u8 p44[0x10];
} E; /* 0x54 */

/* callers read v0: none-direct; void. */
extern void func_801298F4(E *e);
extern void func_801299C8(s32 a, s32 b, E *e);
extern u32 D_8005128C[];
extern s16 D_80114EE0;
extern E D_800B9A78[];

void func_8012944C(void) {
    s32 i;
    E *p;

    if (D_8005128C[0] == 0x34420200) {
        D_8005128C[0] = 0;
    }
    D_80114EE0 = 1;
    p = D_800B9A78;
    for (i = 0; i < 2; p++, i++) {
        switch (p->f34) {
        case 0x7FFD:
            if (i == 0) {
                func_801299C8(1, 1, p);
            }
            break;
        case 0x7FFE:
            func_801299C8(1, (s16)(i * 2), p);
            break;
        case 0x7FFF:
            break;
        default:
            func_801298F4(p);
            p->f1E = p->f42 + p->f3A;
            func_801299C8(1, (s16)(i + 3), p);
            break;
        }
    }
}
