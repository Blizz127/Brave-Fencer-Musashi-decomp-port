/* Owner re-derivation from retail assembly (clean input set). */
/* Decrements the byte counter at offset 0x1BE if nonzero. */
#include "psx_types.h"

typedef struct {
    u8 pad[446];
    u8 cnt; /* 0x1BE */
} Obj8014a71c;

void func_8014A71C(Obj8014a71c *o)
{
    if (o->cnt != 0) {
        o->cnt--;
    }
}
