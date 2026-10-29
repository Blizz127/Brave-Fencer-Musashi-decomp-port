/* Owner re-derivation from retail assembly (clean input set). */
/* Calls the object refresh helper when the object's 16-bit field at 0xFC differs from field 0x36 of the record at offset 0x64. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x36];
    s16 f36;
} Rec;

typedef struct {
    u8 pad0[0x64];
    u32 f64;
    u8 pad1[0xFC - 0x68];
    s16 fFC;
} Obj;

extern void func_8012C218(Obj *o);

void func_80182A7C(Obj *o)
{
    if (o->fFC != ((Rec *)o->f64)->f36) {
        func_8012C218(o);
    }
}
