/* Owner re-derivation from retail assembly (clean input set). */
/* Thunk: runs func_8012A62C with the low byte of its second argument, then tail-calls the
 * caller-supplied callback (its first argument, a bare function pointer). */
#include "psx_types.h"

typedef void (*Callback)(void);

extern void func_8012A62C(u8 v);

void func_8012A5F8(Callback cb, u16 v)
{
    func_8012A62C((u8)v);
    cb();
}
