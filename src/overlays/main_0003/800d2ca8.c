/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

extern s32 func_800D2F88(void);
extern u32 func_800D30B8(void);
s32 func_800D2CA8(s32 arg0) {
 s32 value;
 if (arg0 == 0) value = func_800D2F88();
 else value = (func_800D30B8() >> 29) & 1;
 return value;
}
