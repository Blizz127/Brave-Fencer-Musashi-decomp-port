/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x2C];
    u32 f2C;
    u8 pad30[0x14];
    u32 f44;
    u8 pad48[0x61];
    u8 fA9;
    u8 padAA[0x192];
    u32 f23C;
} Obj8015A2D8;

extern u8 D_80180990[];
extern void func_80146DB8(void *o, void *tbl);
extern void func_80154150(void *o, s32 i);
extern void func_80154A74(void *o, s32 i);
extern void func_8014C010(void *o, s32 i);
extern void func_801553A8(void *o);
extern void func_80172608(void *o);
extern void func_80146CA0(void *o);

void func_8015A2D8(Obj8015A2D8 *o) {
    func_80146DB8(o, D_80180990);
    if (o->f44 & 0x200) {
        func_80154150(o, 0xA);
        func_80154A74(o, 0x1C);
        o->f2C = 0xFFFC0000;
    } else {
        switch (o->fA9) {
        case 0x41:
            func_80154150(o, 7);
            func_80154A74(o, 0x21);
            break;
        case 0x53:
        case 0x73:
            func_80154150(o, 6);
            func_80154A74(o, 0x21);
            break;
        default:
            break;
        }
    }
    o->f23C = 0;
    func_8014C010(o, 2);
    func_801553A8(o);
    func_80172608(o);
    func_80146CA0(o);
}
