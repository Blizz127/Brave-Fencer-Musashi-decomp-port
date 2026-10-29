/* Owner re-derivation from retail assembly (clean input set). */
/* Command dispatcher: runs one of forty-odd sound/event/transition commands selected by a command code. */
#include "psx_types.h"

extern u8 D_800A4F1B;
extern s16 D_800A4EE0;
extern u8 D_800A4F1C;
extern u8 D_8006A96E;
extern u8 D_8006A980;
extern s16 D_8006A982;
extern u8 D_8006A984;
extern u8 D_800A4EE6;
extern s16 D_800A4EFC;
extern u8 D_800A4F1E;
extern s16 D_800A4EF6;
extern u8 D_800A4F19;
extern u8 D_8006A994[];
extern u8 D_800A4F16;
extern s16 D_800A4EF8;
extern s16 D_800A4EFA;
extern u16 D_800A4E8C;
extern s16 D_800A4E86;
extern u16 D_800A4E8E;
extern s16 D_8006A990;
extern s16 D_800A4F20;

extern void func_8002E638(u32);
extern void func_8002E700(u32);
extern void func_8002F0D0(void);
extern void func_8002EDE4(void);
extern void func_80031CC8(void);
extern void func_8002EED8(u32);
extern void func_8002EE90(void);
extern void func_8002E5BC(void);
extern void func_8002E5F8(void);
extern void func_8003491C(u32);
extern void func_8002EB10(void);
extern void func_8002EBAC(void);
extern void func_8002E79C(u32);
extern void func_8002E8DC(u32);
extern void func_80031F14(void);
extern void func_8002E818(s32);
extern void func_80031FC8(void);
extern void func_8002E94C(s32);
extern void func_8002EEB8(void);
extern void func_8002EC10(void);
extern void func_80031DEC(void);
extern void func_80031E94(void);
extern void func_8002EFD0(void);
extern void func_80038668(s32, s32);
extern void func_8002D904(u32);

void func_8002E138(u16 arg0, u32 arg1, u32 arg2) {

    if (arg0 == 0x7F) {
        return;
    }
    switch (arg0) {
    case 1:
        func_8002E638(arg1 & 0xFFFF);
        break;
    case 8:
        func_8002E700(arg1 & 0xFFFF);
        break;
    case 2:
        func_8002F0D0();
        func_8002EDE4();
        D_800A4F1B = 1;
        D_800A4EE0 = 0x4000;
        D_800A4F1C = 0;
        D_8006A96E = 0;
        D_8006A980 = 0;
        D_8006A982 = 0;
        D_8006A984 = 0;
        D_800A4EE6 = 0;
        D_800A4EFC = 0x1E;
        D_800A4F1E = 0;
        break;
    case 3:
        func_8002F0D0();
        func_80031CC8();
        D_800A4EFC = 0;
        break;
    case 4:
        func_8002EED8((arg1 & 0xFFFF) | (arg2 << 16));
        break;
    case 5:
        func_8002EE90();
        break;
    case 6:
    case 16:
    case 31:
        D_800A4EF6 = 2;
        break;
    case 7:
    case 17:
    case 32:
        D_800A4EF6 = 1;
        break;
    case 11:
        if (D_800A4F19 != 0) {
            func_8002E5BC();
        }
        break;
    case 12:
        if (D_800A4F19 == 0) {
            func_8002E5F8();
        }
        break;
    case 13:
        func_8003491C((arg1 & 0xFFFF) | (arg2 << 16));
        break;
    case 14:
        func_8002EB10();
        break;
    case 15:
        func_8002EBAC();
        break;
    case 18:
        func_8002E79C(arg1 & 0xFFFF);
        break;
    case 19:
        func_8002E8DC(arg1 & 0xFFFF);
        break;
    case 20:
        func_80031F14();
        func_8002E818(0);
        break;
    case 21:
        func_80031FC8();
        func_8002E94C(0);
        break;
    case 22:
        func_8002EEB8();
        break;
    case 23:
        func_8002EC10();
        break;
    case 24:
    case 26:
        func_80031DEC();
        break;
    case 25:
    case 27:
        func_80031E94();
        break;
    case 28:
        func_8002EFD0();
        break;
    case 29:
        D_800A4F1B = 0;
        break;
    case 30:
        D_800A4F1B = 1;
        break;
    case 33:
        D_800A4EF8 = D_8006A994[arg1 & 7];
        D_800A4F16 = 1;
        break;
    case 34:
        D_800A4EFA = D_8006A994[arg1 & 7];
        break;
    case 35:
    {
        u8 *q = &D_800A4F1C;
        *q |= 1;
        if (D_800A4E8C + 0x100 == 0x134) {
            func_80038668(D_800A4E86, 3);
        }
        break;
    }
    case 36:
    {
        u8 *q = &D_800A4F1C;
        *q |= 2;
        if (D_800A4E8C + 0x100 == 0x134) {
            func_80038668(D_800A4E86, 4);
        }
        break;
    }
    case 37:
    {
        u8 *q = &D_800A4F1C;
        *q |= 4;
        if (D_800A4E8C + 0x100 == 0x134) {
            func_80038668(D_800A4E86, 5);
            func_80038668(D_800A4E86, 6);
            func_80038668(D_800A4E86, 7);
        }
        break;
    }
    case 38:
    {
        u8 *q = &D_800A4F1C;
        *q |= 8;
        if (D_800A4E8C + 0x100 == 0x134) {
            func_80038668(D_800A4E86, 8);
        }
        break;
    }
    case 39:
        if (D_800A4E8E & 1) {
            D_8006A990 = 0;
            D_800A4F20 = arg1;
        } else {
            func_8002D904(arg1 & 0xFFFF);
        }
        break;
    case 42:
        D_800A4F1C = 0;
        break;
    case 40:
        D_800A4F1E = 1;
        break;
    case 41:
        D_800A4F1E = 0;
        func_80031CC8();
        break;
    }
}
