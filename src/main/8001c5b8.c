/* Owner re-derivation from retail assembly (clean room, clean input set). State view: D_800AF630 + 0x198/0x19C = D_800AF7C8/D_800AF7CC; func_8001C9D0 takes the object (stores through a0). */
/* func_8001C5B8: initialise object `o` and register every entry of the zero-terminated model list `list`. */
#include "psx_types.h"

typedef struct {
    s16 a;
    s16 b;
    u8 pad4[0x1C];
    u32 table;
    u32 list;
    u8 pad28[6];
    s16 idx;
    u8 mat[0x20];
} Obj;

extern u8 D_800AF630[];
extern u8 D_800A5BD0[];
extern u8 D_80063548[];
typedef struct {
    u8 pad0[0x198];
    u32 acc;
    u16 idx;
} State;

extern void func_8001C9D0(Obj *);
extern void func_80052D90(s32, void *);
extern void func_80054514(void *, void *);
extern void func_80053290(void *);
extern void func_800553D8(void *, void *, s32);
extern u32 func_800553F4(void *, u32);

void func_8001C5B8(Obj *o, u32 *list) {
    u8 buf[0x20];
    u32 acc;
    State *state;
    u8 *p;
    u8 *rec;
    u32 i;

    func_8001C9D0(o);
    o->a = 1;
    o->b = 8;
    func_80052D90(0, o->mat);
    func_80054514(o->mat, buf);
    state = (State *)D_800AF630;
    o->list = (u32)list;
    if (list != 0) {
        o->table = (u32)D_80063548;
        acc = state->acc;
        o->idx = state->idx;
        while (*list != 0) {
            p = (u8 *)(*list++ & 0x80FFFFFF);
            p += 4;
            func_80053290(p);
            i = state->idx;
            rec = D_800A5BD0 + i * 20;
            state->idx = i + 1;
            func_800553D8(p + 8, rec, 0);
            acc = func_800553F4(rec, acc);
        }
        state->acc = acc;
    }
}
