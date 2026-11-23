#ifndef BFM_CONTROLLER_REPORT_H
#define BFM_CONTROLLER_REPORT_H

#include "psx_types.h"

typedef struct {
    u8 type;
    u8 pad1[0x2A - 1];
    u8 current[8];
    u8 previous[8];
    u8 pressed[8];
    u8 older[8];
    u8 act0;
    u8 act1;
} PadRecord;

extern PadRecord D_80078D98[2];
s32 func_80018A20(s32 index);
void func_80018C64(PadRecord *record);
void func_80018CE8(PadRecord *record);
void func_80018E9C(void *record);
void func_80018F88(PadRecord *record);

#endif
