/* Owner re-derivation from retail assembly (clean input set). */
/* Builds the four status panels' text: for each slot copies the seven template strings into the slot's buffers, then fills the numeric fields (level, time, counters) as BCD digits; empty slots get placeholders. */
#include "psx_types.h"

typedef struct {
    u8 c[3];
} Rgb3;

typedef struct {
    s32 w0;        /* 0x00 */
    u8 pad04[2];
    u8 b6;         /* 0x06 */
    u8 b7;         /* 0x07 */
    s16 h8;        /* 0x08 */
    u8 pad0A[2];
    u16 h0C;       /* 0x0C */
    u8 pad0E[6];
    u32 w14;       /* 0x14 */
    u8 pad18[0x1C];
    u16 h34;       /* 0x34 */
    u8 pad36[2];
    u8 b38;        /* 0x38 */
    u8 b39;        /* 0x39 */
    u16 h3A;       /* 0x3A */
    u16 h3C;       /* 0x3C */
    u16 h3E;       /* 0x3E */
    u16 h40;       /* 0x40 */
    u8 b42;        /* 0x42 */
    u8 pad43[0xB];
    u8 b4E;        /* 0x4E */
} SlotStatus;

extern u32 D_80185AF0[];   /* guest address of each slot's SlotStatus (0 = empty) */
extern u32 D_80182614[];   /* guest address of each slot's 7-entry buffer table */
extern u32 D_8018251C[];   /* guest addresses of the 7 template strings */
extern u32 D_8018253C;     /* guest address of the empty-slot string for field 4 */
extern s32 D_801827A8[];   /* 5 thresholds for field 0 */
extern u32 D_80182580[];   /* guest addresses of 7 u16 triples */
extern Rgb3 D_80183F30;

extern u8 *func_8005C540(u8 *dst, u8 *src);
extern u32 func_8017DC48(s32 value, s32 shift);
extern void func_8017DCB0(u32 val, s16 digits, u16 *out, s16 zero_pad);

void func_8017D730(void)
{
    s32 *thr;
    u32 *tmpl;
    s16 s;
    s16 j;
    s16 k;
    SlotStatus *rec;
    u32 *dst;
    u32 n;
    u32 m;
    u32 val;
    s32 idx;
    u16 *p;

    for (s = 0; s < 4; s++) {
        thr = D_801827A8;
        tmpl = D_8018251C;
        rec = (SlotStatus *)D_80185AF0[s];
        dst = (u32 *)D_80182614[s];
        if (rec != 0) {
            for (j = 0; j < 7; j++) {
                func_8005C540((u8 *)dst[j], (u8 *)tmpl[j]);
                switch (j) {
                case 0:
                    for (k = 0; k < 5; k++) {
                        if (rec->h8 < thr[k]) {
                            break;
                        }
                    }
                    if (rec->b4E) {
                        if (rec->w0 == 0) {
                            k = 0x1A;
                        } else {
                            k = 0x1C;
                        }
                    }
                    dst[0] = k + 4;
                    break;
                case 1:
                    n = rec->b7;
                    if (n >= 100) {
                        n = 99;
                    }
                    m = rec->b6;
                    if (m >= 60) {
                        m = 59;
                    }
                    val = func_8017DC48((u16)n, 0x18) | 0xB00000 | func_8017DC48(m, 0xC);
                    func_8017DCB0(val, 5, (u16 *)(dst[1] + 0x12), 0);
                    break;
                case 2:
                    n = rec->h34;
                    if (n >= 1000) {
                        n = 999;
                    }
                    val = func_8017DC48((u16)n, 0x14);
                    func_8017DCB0(val, 3, (u16 *)(dst[2] + 8), 1);
                    idx = rec->b42;
                    if (idx > 6) {
                        idx = 6;
                    }
                    p = (u16 *)D_80182580[idx];
                    ((u16 *)dst[2])[8] = *p++;
                    ((u16 *)dst[2])[9] = *p++;
                    ((u16 *)dst[2])[10] = *p;
                    n = rec->b39;
                    if (n >= 24) {
                        n = 23;
                    }
                    m = rec->b38;
                    if (m >= 60) {
                        m = 59;
                    }
                    func_8017DCB0(func_8017DC48((u16)n, 0x18) | 0xB00000 | func_8017DC48(m, 0xC), 5, (u16 *)(dst[2] + 0x18), 0);
                    break;
                case 3:
                    n = rec->h3C;
                    if (n >= 1000) {
                        n = 999;
                    }
                    m = rec->h3A;
                    if (m >= 1000) {
                        m = 999;
                    }
                    val = func_8017DC48((u16)n, 0x14);
                    if (val == 0) {
                        val = 0xD00000;
                    }
                    func_8017DCB0(val | (func_8017DC48(m, 4) | 0xC0000), 7, (u16 *)(dst[3] + 6), 1);
                    break;
                case 4:
                    n = rec->h40;
                    if (n >= 1000) {
                        n = 999;
                    }
                    m = rec->h3E;
                    if (m >= 1000) {
                        m = 999;
                    }
                    val = func_8017DC48((u16)n, 0x14);
                    if (val == 0) {
                        val = 0xD00000;
                    }
                    func_8017DCB0(val | (func_8017DC48(m, 4) | 0xC0000), 7, (u16 *)(dst[4] + 6), 1);
                    break;
                case 5:
                    n = rec->w14;
                    if (n > 99999) {
                        n = 99999;
                    }
                    val = func_8017DC48(n, 0xC);
                    func_8017DCB0(val, 5, (u16 *)(dst[5] + 2), 1);
                    break;
                case 6:
                    n = rec->h0C;
                    if (n >= 36) {
                        n = 35;
                    }
                    val = func_8017DC48((u16)n, 0x18);
                    if (val == 0) {
                        val = 0xD000000;
                    }
                    func_8017DCB0(val, 2, (u16 *)(dst[6] + 0x10), 1);
                    break;
                }
            }
        } else {
            for (j = 0; j < 7; j++) {
                val = 4;
                if (j == 0) {
                    dst[0] = val;
                } else if (j == 1) {
                    func_8005C540((u8 *)dst[1], (u8 *)tmpl[1]);
                } else if (j == val) {
                    func_8005C540((u8 *)dst[4], (u8 *)D_8018253C);
                } else {
                    *(Rgb3 *)dst[j] = D_80183F30;
                }
            }
        }
    }
}
