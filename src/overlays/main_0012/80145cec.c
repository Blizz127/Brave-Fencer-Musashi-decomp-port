/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x18];
    s16 f18;
    s16 f1A;
    s16 f1C;
    u8 pad1E[0x0A];
    u32 f28;
    u16 f2C;
} Mdl80145CEC;

typedef struct {
    u8 pad00[0x20];
    u32 model;
    u8 pad24[0x14];
    u32 f38;
    u8 pad3C[0x24];
    s16 f60;
    s16 f62;
    s16 f64;
    u8 pad66[2];
    s16 f68;
    s16 f6A;
    s16 f6C;
    u8 pad6E[2];
    s16 f70;
    s16 f72;
    s16 f74;
    u8 pad76[0x80];
    s16 fF6;
    u8 padF8[0xC0];
    s16 f1B8;
    s16 f1BA;
    u8 pad1BC[0x98];
} Player80145CEC; /* size 0x254 */

extern Player80145CEC D_80126B58;
extern u8 D_800AFAE8[];
extern u8 D_801152C8[];
extern u8 D_800DE2A4[];
extern u8 D_8017FD30[];
extern u32 D_800AF7C8;
extern s16 D_800B9A08;
extern s32 D_8011F9C4;
extern s32 D_8012707C;
extern s32 D_8011DB10;
extern void func_80016714(void *p, s32 n);
extern void func_8001C320(void *a, void *b);
extern void func_801552F4(void *o);
extern void func_80147084(void *o);
extern void func_80147098(void *o);
extern void func_801470AC(void *o);
extern void func_80149210(void *o, s32 a);
extern void func_8014BE9C(void);
extern void func_8016533C(void);
extern void func_801627C0(void);
extern void func_80162AF4(void);
extern void func_80165C78(void);
extern void func_801468C8(void *o, s32 a);
extern void func_80145EE8(s32 a);
extern void func_8014C968(void);
extern void func_8014C6D0(void);
extern void func_80165938(void);
extern void func_80153C8C(void);
extern u32 func_80029504(void);
extern void func_800290BC(void);

void func_80145CEC(void) {
    s32 unused_frame[2];

    func_80016714(&D_80126B58, 0x254);
    D_80126B58.model = (u32)D_800AFAE8;
    D_800AF7C8 = (u32)D_801152C8;
    func_8001C320(D_800AFAE8, D_800DE2A4);
    ((Mdl80145CEC *)D_80126B58.model)->f28 = 0x7FFF7FFF;
    D_80126B58.f38 = (u32)D_8017FD30;
    ((Mdl80145CEC *)D_80126B58.model)->f18 = 0x1000;
    D_80126B58.f70 = 0x1000;
    D_80126B58.f68 = 0x1000;
    D_80126B58.f60 = 0x1000;
    ((Mdl80145CEC *)D_80126B58.model)->f1A = 0x1000;
    D_80126B58.f72 = 0x1000;
    D_80126B58.f6A = 0x1000;
    D_80126B58.f62 = 0x1000;
    ((Mdl80145CEC *)D_80126B58.model)->f1C = 0x1000;
    D_80126B58.f74 = 0x1000;
    D_80126B58.f6C = 0x1000;
    D_80126B58.f64 = 0x1000;
    D_80126B58.f1BA = 0x1000;
    D_80126B58.f1B8 = 0x1000;
    D_80126B58.fF6 = 0x1000;
    func_801552F4(&D_80126B58);
    func_80147084(&D_80126B58);
    func_80147098(&D_80126B58);
    func_801470AC(&D_80126B58);
    ((Mdl80145CEC *)D_80126B58.model)->f2C |= 0x50;
    func_80149210(&D_80126B58, 8);
    func_8014BE9C();
    func_8016533C();
    func_801627C0();
    func_80162AF4();
    func_80165C78();
    func_801468C8(&D_80126B58, 1);
    if (D_800B9A08 != 0x30A2 && D_800B9A08 != 0x30A5) {
        func_80145EE8(0);
    }
    func_8014C968();
    func_8014C6D0();
    D_8011F9C4 = 0;
    func_80165938();
    func_80153C8C();
    D_8012707C = 0;
    D_8011DB10 = 0;
    if (func_80029504() >= 0xA) {
        func_800290BC();
    }
}
