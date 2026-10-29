/* Owner re-derivation from retail assembly (clean input set). */
/* Decrements the byte counter at offset 0x223 if nonzero. */
#include "psx_types.h"

typedef struct {
    u8 pad[547];
    u8 cnt; /* 0x223 */
} Obj80149bec;

void func_80149BEC(Obj80149bec *o)
{
    if (o->cnt != 0) {
        o->cnt--;
    }
}
