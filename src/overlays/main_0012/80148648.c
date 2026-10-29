/* Owner re-derivation from retail assembly (clean input set). */
/* Loads an actor's pad state (button byte, sticks, analog word) for a port, applies overrides, and remaps bits. */
#include "psx_types.h"

typedef struct {
    /* 0x000 */ u16 state;
    /* 0x002 */ u8 pad002[0xA7];
    /* 0x0A9 */ u8 kind;
    /* 0x0AA */ u16 btn;
    /* 0x0AC */ u16 btn2;
    /* 0x0AE */ u16 analog;
    /* 0x0B0 */ u8 pad0B0[0x100];
    /* 0x1B0 */ s32 remap;
    /* 0x1B4 */ u8 pad1B4[0x11];
    /* 0x1C5 */ u8 locked;
} Obj80148648;

extern u8 D_800B9A64;

extern u8 func_80014DC0(u8 port);
extern u16 func_80014D68(u8 port);
extern u16 func_80014D94(u8 port);
extern u8 func_80014CF8(u8 port, u8 idx);
extern void func_800120DC(u16 *a, u16 *b);
extern s32 func_800CF8B4(void);

void func_80148648(Obj80148648 *obj, u8 port)
{
    u8 lo;
    u8 hi;
    u32 a;
    u32 b;
    u32 c;
    u32 k;

    if (D_800B9A64 == 0) {
        obj->kind = func_80014DC0(port);
        obj->btn = func_80014D68(port);
        obj->btn2 = func_80014D94(port);
        lo = func_80014CF8(port, 4);
        hi = func_80014CF8(port, 5);
        obj->analog = lo | (hi << 8);
    } else {
        obj->kind = 0x41;
        func_800120DC(&obj->btn, &obj->btn2);
    }
    if (obj->locked != 0) {
        obj->btn = 0;
        obj->btn2 = 0;
        obj->analog = 0x8080;
    }
    if (func_800CF8B4() == 0 && obj->state != 0x1E && obj->state != 0x1A) {
        obj->btn = 0;
        obj->btn2 = 0;
        obj->analog = 0x8080;
    }
    switch (obj->kind) {
    case 0x53:
    case 0x73:
        if (obj->btn & 0xF000) {
            obj->kind = 0x41;
        }
        break;
    case 0x41:
        break;
    }
    if (obj->remap != 0) {
        a = obj->btn;
        c = obj->analog;
        b = obj->btn2;
        obj->btn = ((a & 0x2000) << 2) | ((s32)(a & 0x8000) >> 2) | (a & 0x5FFF);
        obj->btn2 = ((b & 0x2000) << 2) | ((s32)(b & 0x8000) >> 2) | (b & 0x5FFF);
        k = c & 0xFF;
        if (k != 0x80) {
            c = (~k & 0xFF) | (c & 0xFF00);
        }
        obj->analog = c;
    }
}
