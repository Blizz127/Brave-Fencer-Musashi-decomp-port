/* Fresh own-retail stores using the qualified object declaration. */
#include "psx_types.h"
typedef struct { u8 pad[0x10]; s32 a, b, c; } Obj801473dc;

void func_801473DC(Obj801473dc *o)
{
    o->c = 0;
    o->b = 0;
    o->a = 0;
}
