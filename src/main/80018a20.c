/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame controller port service: checks pad state and, on port 0, resets lost pads and sets up the actuator (vibration) parameters. */
#include "psx_types.h"

/* The default switch arm falls off the end of this int function. Retail then leaves a non-zero v0
 * (1 or 6), and the only caller tests v0 != 0. Host builds return 1 there. MIPS bytes are unchanged. */
#ifdef __mips__
#define RETAIL_NONZERO_EXIT
#else
#define RETAIL_NONZERO_EXIT return 1
#endif

typedef struct {
    u8 pad[0x4A];
    u8 act0;
    u8 act1;
} PadRecord;

extern PadRecord D_80078D98[];
extern u32 D_800AE610;
extern s32 D_800A5E78;
extern u8 D_80062BAC[];
extern u8 D_80062BB4[];
extern u8 D_80062BBC;
extern u8 D_80062BBD;
extern u8 D_800747B8;
extern u8 D_800747B9;

extern s32 func_8005D184(s32 port);
extern s32 func_8005D244(s32 port, s32 term, s32 offs);
extern void func_8005D538(s32 port, u8 *data, s32 len);
extern void func_8005D4B8(s32 port, u8 *data);
extern void func_80018F88(PadRecord *rec);
extern void func_80018FC8(void);
extern s32 func_80028D58(void);

s32 func_80018A20(s32 index)
{
    PadRecord *rec;
    s32 port;

    rec = &D_80078D98[index];
    switch (index) {
    case 0:
        port = 0;
        break;
    case 1:
        port = 0x10;
        break;
    }
    switch (func_8005D184(port)) {
    case 0:
        if (index == 0) {
            if (D_800AE610++ >= 10) {
                D_800AE610 = 0;
                D_800A5E78 = 0;
            }
        }
        /* fall through */
    case 1:
        func_80018F88(rec);
        return 0;
    case 2:
        if (index != 0) {
            return 1;
        }
        D_800AE610 = 0;
        if (func_8005D244(port, 2, 0) != 0) {
            return 1;
        }
        if (func_80028D58() == 0) {
            if (rec->act0 | rec->act1) {
                D_80062BBD = 1;
            } else {
                D_80062BBD = 0;
            }
            rec->act1 = 0;
            rec->act0 = 0;
        } else {
            func_80018FC8();
        }
        func_8005D538(port, &D_80062BBC, 2);
        func_8005D4B8(port, D_80062BAC);
        return 1;
    case 6:
        if (index != 0) {
            return 1;
        }
        D_800AE610 = 0;
        if (func_8005D244(port, 2, 0) == 0) {
            return 1;
        }
        if (func_80028D58() == 0) {
            D_800747B8 = rec->act0;
            D_800747B9 = rec->act1;
            rec->act1 = 0;
            rec->act0 = 0;
        } else {
            func_80018FC8();
        }
        func_8005D538(port, &D_800747B8, 2);
        func_8005D4B8(port, D_80062BB4);
        return 1;
    }
    RETAIL_NONZERO_EXIT;
}
