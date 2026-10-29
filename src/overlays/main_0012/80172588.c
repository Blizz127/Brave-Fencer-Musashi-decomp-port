/* Owner re-derivation from retail assembly (clean input set). */
/* Clears the word at +0x1F4 of the object. */
#include "psx_types.h"

typedef struct {
    u8 pad000[0x1F4];
    u32 word1F4;
} Obj80172588;

void func_80172588(Obj80172588 *obj)
{
    obj->word1F4 = 0;
}
