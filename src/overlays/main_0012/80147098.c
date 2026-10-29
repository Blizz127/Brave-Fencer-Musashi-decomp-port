/* Owner re-derivation from retail assembly (clean input set). */
/* Clears the byte flag at +0x220 of the object. */
#include "psx_types.h"

typedef struct {
    u8 pad000[0x220];
    u8 flag220;
} Obj80147098;

void func_80147098(Obj80147098 *obj)
{
    obj->flag220 = 0;
}
