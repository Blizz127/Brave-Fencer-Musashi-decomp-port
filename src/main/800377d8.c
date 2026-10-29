/* Owner re-derivation from retail assembly (clean input set). */
/* Streaming state machine: reads the next sector of the current channel and advances header/body/tail phases. */
#include "psx_types.h"

typedef struct {
    u32 f0;
    u32 f4;
    u32 f8;
    u8 fC;
    u8 active;
    u8 kind;
    u8 padF;
} Chan;

extern u32 D_80076228;
extern u32 D_80076238;
/* Same storage as D_8007623C..D_80076243, viewed as one record. */
typedef struct {
    s32 seq;
    s16 cnt;
    u8 f1A;
    u8 state;
} Hdr;
extern Hdr D_8007623C;
extern s16 D_80076240;
extern u8 D_80076242;
extern u8 D_80076243;
extern Chan D_80076244[];
extern s32 D_80076294;
extern u8 D_80076298;
extern u8 D_800762A0[];
extern u8 D_8006AEF4;
extern s32 func_80043994(u32, s32);
extern s32 func_80043B1C(u8 *);
extern s32 func_8003C4F0(s32);
extern s32 func_8003C498(u32);
extern void func_8003C438(u32, s32);

void func_800377D8(u8 arg)
{
    s32 v;

    if (arg != 1) goto fail;
    if (func_80043994((u32)D_800762A0, 3) == 0) goto fail;
    v = func_80043B1C(D_800762A0);
    if (v != D_8007623C.seq) goto fail;
    D_8007623C.seq = v + 1;
    switch (D_8007623C.state) {
    case 1:
        if (func_80043994(D_80076228, 0x200) == 0) goto fail;
        if (D_80076240 == 0 && *(u32 *)D_80076228 != 0x7671732E) {
            D_80076298 = 1;
            D_80076244[D_80076294].fC = 2;
            goto fail;
        }
        D_80076228 += 0x800;
        D_8007623C.cnt++;
        if (D_8007623C.cnt != 14) return;
        if (D_80076244[D_80076294].f4 & 0x1000) {
            D_80076244[D_80076294].fC = 1;
            D_80076294++;
            if (D_80076294 < 5 && D_80076244[D_80076294].active != 0) {
                if (D_80076244[D_80076294].f4 & 0x2000) {
                    D_8007623C.state = 1;
                    D_80076298 = 0;
                    D_80076240 = 0;
                } else {
                    D_80076243 = 2;
                    D_80076298 = 0;
                    D_80076240 = 0;
                }
                D_80076228 = D_80076244[D_80076294].f0;
                D_80076238 = D_80076244[D_80076294].f8;
                return;
            }
            D_80076243 = 4;
            D_8006AEF4 &= 0xFD;
            return;
        }
        D_80076243 = 2;
        D_8007623C.cnt = 0;
        return;
    case 2:
        if (func_80043994(D_80076228, 0x200) == 0) goto fail;
        D_80076228 += 0x800;
        D_80076240++;
        if (D_80076240 != 7) return;
        D_80076243 = 3;
        D_80076240 = 0;
        if (D_80076242 == 0) return;
        if (func_8003C4F0(0) == 0) return;
        D_80076242 = 0;
        return;
    case 3:
        if (func_80043994(D_80076228, 0x200) == 0) goto fail;
        D_80076240++;
        if (D_80076242 != 0) {
            if (func_8003C4F0(0) == 0) goto fail;
        }
        if (func_8003C498(D_80076238) == 0) goto fail;
        if (D_80076240 == ((s16 *)D_80076228)[-2]) {
            func_8003C438(D_80076228, ((s16 *)D_80076228)[-1]);
            D_80076242 = 1;
            D_8006AEF4 |= 1;
            D_80076244[D_80076294].fC = 1;
            D_80076294++;
            if (D_80076294 < 5 && D_80076244[D_80076294].active != 0) {
                if (D_80076244[D_80076294].f4 & 0x2000) {
                    D_80076243 = 1;
                    D_80076298 = 0;
                    D_80076240 = 0;
                } else {
                    D_80076243 = 2;
                    D_80076298 = 0;
                    D_80076240 = 0;
                }
                D_80076228 = D_80076244[D_80076294].f0;
                D_80076238 = D_80076244[D_80076294].f8;
                return;
            }
            D_80076243 = 4;
            D_8006AEF4 &= 0xFD;
            return;
        }
        func_8003C438(D_80076228, 0x800);
        D_80076242 = 1;
        D_8006AEF4 |= 1;
        D_80076238 += 0x800;
        return;
    }
    return;
fail:
    if (D_8007623C.state == 0) return;
    if (D_8007623C.state == 4) return;
    if (D_8007623C.state == 5) return;
    D_8007623C.state = 5;
    D_8006AEF4 &= 0xFD;
}
