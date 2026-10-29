/* Owner re-derivation from retail assembly (clean input set). */
/* Disc check state machine: polls the drive, sets the drive mode, retries the file search; returns 1 once the file is found. */
#include "psx_types.h"

/* Drive command buffers: result bytes at D_800AE738, parameter bytes at D_800AE740. */
typedef struct {
    u8 result[8];
    u8 param[8];
} CdCmdBuf;

/* File record: path string at D_80063014, search result block at D_80063028. */
typedef struct {
    char path[0x14];
    u8 info[0x1C];
} DiscFile;

extern s32 D_800AE6F4;
extern CdCmdBuf D_800AE738;
extern s32 D_800AE750[];
extern DiscFile D_80063014;
extern void func_800434BC(void);
extern s32 func_80043830(u8, u8 *, u8 *);
extern s32 func_80045374(u8 *, char *);
extern s32 func_80046630(s32);
extern s32 func_8004674C(void);

s32 func_8001A114(void)
{
    s32 ret;
    s32 i;
    s32 r;

    ret = 0;
    switch (D_800AE6F4) {
    case 0:
        func_800434BC();
        func_80043830(1, 0, D_800AE738.result);
        if (D_800AE738.result[0] & 0x10) break;
        D_800AE6F4++;
    case 1:
        r = func_80046630(0);
        if (r == 2) {
            /* Bumped through its address: retail keeps that address in a register for this step. */
            (*(u32 *)&D_800AE6F4)++;
        } else if (r == 0x10) {
            D_800AE6F4 = 0;
        }
        break;
    case 2:
        D_800AE738.param[0] = 0x80;
        if (func_80043830(0xE, D_800AE738.param, D_800AE738.result) == 0) break;
        if (D_800AE738.result[0] & 0x10) {
            D_800AE6F4 = 0;
            break;
        }
        D_800AE750[0] = 0;
        D_800AE6F4++;
        break;
    case 3:
        if (++D_800AE750[0] >= 3) {
            D_800AE750[0] = 0;
            D_800AE6F4++;
        }
        break;
    case 4:
        r = func_8004674C();
        if (r == 1 || r == 0x10 || r == 0) D_800AE6F4 = 0;
        if (r == 2) (*(u32 *)&D_800AE6F4)++;
        break;
    case 5:
        for (i = 0; i < 16; i++) {
            r = func_80045374(D_80063014.info, D_80063014.path);
            if (r != -1) break;
        }
        if (r == -1 || r == 0) {
            D_800AE6F4 = 0;
            break;
        }
        ret = 1;
        break;
    }
    return ret;
}
