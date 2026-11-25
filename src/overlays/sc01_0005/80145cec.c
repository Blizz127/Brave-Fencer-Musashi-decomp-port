/* Owner re-derivation from retail assembly (clean input set). */
/* Scene setup for the field mode: clears the 0x254-byte scene record, points the active-record and
 * list globals at their tables, seeds the 0x1000 default scale values, runs the init passes on the
 * record in order, then arms the pending-event state (calls func_800290BC if the counter is 10 or more). */
#include "psx_types.h"

/* Scene record view: the active-record pointer lives 0x20 bytes into the scene record. */
typedef struct {
    u8 p0[0x20];
    u32 ptr;
} Scene;
#define SCN ((Scene *)D_80126B58)
/* Pointer-valued globals below are guest addresses (u32). */
extern u8 D_80126B58[];
extern u8 D_800AFAE8[];
extern u8 D_800DE2A4[];
extern u8 D_801152C8[];
extern u8 D_80183D60[];
extern u32 D_800AF7C8;
extern u32 D_80126B90;
extern s16 D_80126BC8;
extern s16 D_80126BC0;
extern s16 D_80126BB8;
extern s16 D_80126BCA;
extern s16 D_80126BC2;
extern s16 D_80126BBA;
extern s16 D_80126BCC;
extern s16 D_80126BC4;
extern s16 D_80126BBC;
extern s16 D_80126D12;
extern s16 D_80126D10;
extern s16 D_80126C4E;
extern s16 D_800B9A08;
extern s32 D_8011F9C4;
extern s32 D_8012707C;
extern s32 D_8011DB10;

extern void func_80016714(u8 *p, s32 n);
extern void func_8001C320(u8 *a, u8 *b);
extern void func_801552F4(u8 *rec);
extern void func_80147084(u8 *rec);
extern void func_80147098(u8 *rec);
extern void func_801470AC(u8 *rec);
extern void func_80149210(u8 *rec, s32 v);
extern void func_8014BE9C(void);
extern void func_8016533C(void);
extern void func_801627C0(void);
extern void func_80162AF4(void);
extern void func_80165C78(void);
extern void func_801468C8(u8 *rec, s32 v);
extern void func_80145EE8(s32 v);
extern void func_8014C968(void);
extern void func_8014C6D0(void);
extern void func_80165938(void);
extern void func_80153C8C(void);
extern u32 func_80029504(void);
extern void func_800290BC(void);

void func_80145CEC(void)
{
    u8 pad[8]; /* unused frame space kept to match the retail frame size */

    func_80016714(D_80126B58, 0x254);
    SCN->ptr = (u32)D_800AFAE8;
    D_800AF7C8 = (u32)D_801152C8;
    func_8001C320(D_800AFAE8, D_800DE2A4);
    *(s32 *)(SCN->ptr + 0x28) = 0x7FFF7FFF;
    D_80126B90 = (u32)D_80183D60;
    *(s16 *)(SCN->ptr + 0x18) = 0x1000;
    D_80126BB8 = D_80126BC0 = D_80126BC8 = 0x1000;
    *(s16 *)(SCN->ptr + 0x1A) = 0x1000;
    D_80126BBA = D_80126BC2 = D_80126BCA = 0x1000;
    *(s16 *)(SCN->ptr + 0x1C) = 0x1000;
    D_80126BBC = D_80126BC4 = D_80126BCC = 0x1000;
    D_80126D12 = 0x1000;
    D_80126D10 = 0x1000;
    D_80126C4E = 0x1000;
    func_801552F4(D_80126B58);
    func_80147084(D_80126B58);
    func_80147098(D_80126B58);
    func_801470AC(D_80126B58);
    *(u16 *)(SCN->ptr + 0x2C) |= 0x50;
    func_80149210(D_80126B58, 8);
    func_8014BE9C();
    func_8016533C();
    func_801627C0();
    func_80162AF4();
    func_80165C78();
    func_801468C8(D_80126B58, 1);
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
    if (func_80029504() >= 10) {
        func_800290BC();
    }
}
