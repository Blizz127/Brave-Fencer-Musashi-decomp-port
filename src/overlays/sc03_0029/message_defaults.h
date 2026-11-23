#ifndef TWINPEAK_MESSAGE_DEFAULTS_H
#define TWINPEAK_MESSAGE_DEFAULTS_H

#include "handle.h"

#if !defined(__mips__) || defined(__LP64__)
#define D_80127C0C bfm_twinpeak_tutorial_scratch
#define func_80139634 bfm_twinpeak_tutorial_rect
#define func_80139DC8 bfm_twinpeak_tutorial_clear
#define func_80059888 bfm_twinpeak_tutorial_rect_service
#endif

typedef struct TutorialMessageRect {
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
} TutorialMessageRect;

extern unsigned short D_80127C0C[648];

void func_80139634(TutorialHandleSlot *slot);
void func_80139DC8(void);
void func_80059888(TutorialMessageRect *rect, int arg1, int arg2, int arg3);

#endif
