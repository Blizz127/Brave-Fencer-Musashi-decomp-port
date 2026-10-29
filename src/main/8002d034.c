/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame sound housekeeping: channel timers, frame callback, delayed request and the level slide. */
#include "psx_types.h"

typedef struct Timer {
    s32 count;
    s32 unk4;
    u32 expire;   /* guest function address (native-lane layout rule) */
} Timer;

extern u8 D_800A46BA;
extern Timer D_800A64B0[];
extern u8 D_8006A980;
extern u32 D_800A4F24;
extern u8 D_800A4E70;
extern s32 D_800A4E68;
extern s16 D_800A4E6C;
extern s16 D_800A4E74;
extern s16 D_800A4E76;
extern s16 D_800A4E78;
extern u8 D_800A4E7A;

void func_8002D29C(void);
void func_8002DF80(void);
void func_8002D240(s16);

void func_8002D034(void) {
    Timer *t;
    s32 i;
    s16 goal;
    s16 cur;
    u8 *slide;

    if (D_800A46BA) {
        func_8002D29C();
    }
    t = D_800A64B0;
    for (i = 0; i < 8; i++, t++) {
        if (t->count != 0) {
            if (--t->count == 0) {
                ((void (*)(struct Timer *))t->expire)(t);
            }
        }
    }
    if (D_8006A980 & 0xEF) {
        func_8002DF80();
    }
    if (D_800A4F24 != 0) {
        ((void (*)(void))D_800A4F24)();
    }
    if (D_800A4E70) {
        if (--D_800A4E68 == 0) {
            D_800A4E70 = 0;
            func_8002D240(D_800A4E6C);
        }
    }
    slide = &D_800A4E7A;
    if (*slide) {
        goal = D_800A4E76;
        cur = D_800A4E74;
        if (goal > cur) {
            if (cur < goal - D_800A4E78) {
                D_800A4E74 = cur + D_800A4E78;
                goto done;
            }
        } else {
            if (goal + D_800A4E78 < cur) {
                D_800A4E74 = cur - D_800A4E78;
                goto done;
            }
        }
        D_800A4E74 = goal;
        *slide = 0;
    done:
        func_8002D240(D_800A4E74);
    }
}
