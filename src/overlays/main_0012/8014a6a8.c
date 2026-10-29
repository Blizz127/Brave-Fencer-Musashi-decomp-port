/* Owner re-derivation from retail assembly (clean input set). */
/* Decrements the byte counter at offset 0x1BD if nonzero. */
#include "psx_types.h"

typedef struct {
    u8 pad[445];
    u8 cnt; /* 0x1BD */
} Obj8014a6a8;

void func_8014A6A8(Obj8014a6a8 *o)
{
    if (o->cnt != 0) {
        o->cnt--;
    }
}
