/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern void func_80016714(void *, int);
void func_8001C14C(void) {
 u8 *base = D_800AF630;
 u8 *p;
 for(p=base+0x65a8; (u32)p<(u32)(base+0x6688);p+=0x38) func_80016714(p,0x38);
 for(p=base+0x2a8; (u32)p<(u32)(base+0x4b8);p+=0x84) func_80016714(p,0x84);
}
