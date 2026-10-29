/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"
s32 func_800387C0(u8 **arg0) {
 u8 *base = *arg0;
 u32 a,b,c,d;
 s32 result;
 *arg0=base+1; a=base[0];
 *arg0=base+2; b=base[1];
 *arg0=base+3; c=base[2];
 *arg0=base+4; d=base[3];
 if ((a|(b<<8)|(c<<16)|(d<<24)) == 0x6B72544D) {
  *arg0=base+8;result=0;
 } else result=-1;
 return result;
}
