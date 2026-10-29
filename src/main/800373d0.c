/* Owner re-derivation from retail assembly (clean input set). */
/* Scans the five pending stream slots and starts the first eligible ones. */
#include "psx_types.h"

typedef struct {
    u32 flags;
    u32 unk4;
    u8 ready;
    u8 armed;
    u8 pad[6];
} StreamSlot;

typedef struct {
    s16 id;
    s16 unk2;
    s16 unk4;
    u16 state;
} StreamState;

extern StreamSlot D_80076248[];
extern StreamState D_800A4E88;
extern u8 D_800BA320[];
extern u16 D_80065438[];

void func_8002D7FC(u8 *);
void func_8002FDE8(s32, u8 *);

void func_800373D0(void) {
    StreamState *st;
    u8 *buf;
    s32 i;
    s32 off;

    st = &D_800A4E88;
    buf = D_800BA320;
    for (i = 0; i < 5; i++) {
        if (D_80076248[i].armed && D_80076248[i].ready && (D_80076248[i].flags & 0x3000)) {
            st->id = D_80076248[i].flags & 0xFFF;
            st->state |= 0x20;
            func_8002D7FC(buf);
            if (!(D_80076248[i].flags & 0x1000)) {
                off = st->id * 2;
                func_8002FDE8(*(u16 *)((u8 *)D_80065438 + off), buf + 0x7000);
            }
        }
    }
}
