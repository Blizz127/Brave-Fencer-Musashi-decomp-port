/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 b[8];
} Bytes8;

typedef struct {
    u8 pad[0x90];
    Bytes8 src;
    u8 pad98[0x15C - 0x98];
    Bytes8 k;
} Obj8014C8F0;

void func_8014C8F0(Obj8014C8F0 *o) {
    o->k = o->src;
}
