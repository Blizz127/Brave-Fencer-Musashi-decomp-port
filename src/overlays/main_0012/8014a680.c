/* Owner re-derivation from retail assembly (clean input set). */
/* Decrements the byte counter at offset 0x1BC if nonzero. */
#include "psx_types.h"

typedef struct {
    u8 pad[444];
    u8 cnt; /* 0x1BC */
} Obj8014a680;

void func_8014A680(Obj8014a680 *o)
{
    if (o->cnt != 0) {
        o->cnt--;
    }
}
