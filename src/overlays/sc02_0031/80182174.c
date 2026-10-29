/* Owner re-derivation from retail assembly (clean input set). */
/* When the object's 16-bit field 0xFC is set: state 2 with a 30-tick timer, closes the message window, loads script D_80188F98 for the object, restarts the text system on D_80188EEC and stops sound 0xAC7. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u16 state;
    u8 pad0[0x1C - 0x4];
    s32 f1C;
    u8 pad1[0xFC - 0x20];
    s16 fFC;
} Obj;

extern u8 D_80188F98[];
extern u8 D_80188EEC[];
extern void func_80178BF8(void);
extern void func_801746DC(void);
extern void func_80178CBC(void *a, void *b);
extern void func_8013C98C(void);
extern void func_8013C9C4(void *a);
extern void func_8002D4C8(s32 a, s32 b);

void func_80182174(Obj *o)
{
    if (o->fFC != 0) {
        o->state = 2;
        o->f1C = 0x1E;
        func_80178BF8();
        func_801746DC();
        func_80178CBC(o, D_80188F98);
        func_8013C98C();
        func_8013C9C4(D_80188EEC);
        func_8002D4C8(0xAC7, 0);
    }
}
