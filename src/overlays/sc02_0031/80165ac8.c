/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef void (*Fn)(void *);

extern u8 D_80078EC1;
extern Fn D_80187AC8[];

void func_80165AC8(void *a0) {
    D_80187AC8[D_80078EC1 & 0x7F](a0);
}
