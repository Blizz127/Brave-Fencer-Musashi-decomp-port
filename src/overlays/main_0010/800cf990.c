/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame handler for the three-state screen D_800B99F6: state 0 arms the entry in D_800AE830 chosen by D_80114E74, states 1 and 2 draw the backdrop (func_800CFAD0) and wait for a button / a timeout. */
#include "psx_types.h"

typedef struct {
    s32 w0;
    s32 w1;
} Ent800CF990;

extern void func_8001C0C8(void);
extern void func_80015310(void);
extern s32 func_8001ABBC(s32 a0, s32 a1, void *a2, s32 a3, s32 a4);
extern void func_800167B8(s32 a);
extern void func_80011C10(void);
extern void func_800CFAD0(void);
extern s32 func_800167F0(s32 a);
extern void func_80011B7C(s32 k);
extern u16 D_800B99F6;
extern u16 D_800B9A00;
extern u16 D_800AF7B8;
extern s16 D_80114E74;
extern s16 D_800D3408[];
extern Ent800CF990 D_800AE830[];

void func_800CF990(void)
{
    u16 t;

    switch (D_800B99F6) {
    case 0:
        func_8001C0C8();
        func_80015310();
        if (func_8001ABBC(0, 0, &D_800AE830[D_800D3408[D_80114E74]], 0, 0) != 0) {
            func_800167B8(0);
            func_80011C10();
        }
        break;
    case 1:
        func_800CFAD0();
        if (func_800167F0(0) & 0xFFFF) {
            func_80011C10();
        }
        break;
    case 2:
        func_800CFAD0();
        t = D_800B9A00 + 1;
        D_800B9A00 = t;
        if ((s16)t >= 0x5B) {
            if (D_800AF7B8 != 0) {
                func_80011B7C(6);
            }
        }
        break;
    }
}
