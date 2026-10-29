/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

extern s16 D_800A4688;
extern u8 D_800A4698;

s32 func_8002F658(void) {
 if (D_800A4698 == 0) return D_800A4688;
 return -1;
}
