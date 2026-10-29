/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
/* Dispatches to func_8012E014 when the byte at +0x74 is set, otherwise to func_8012E138. */
void func_8012E014(char *o);
void func_8012E138(char *o);
void func_8012DFD4(char *o) {
    if (*(u8 *)(o + 0x74) != 0) {
        func_8012E014(o);
    } else {
        func_8012E138(o);
    }
}
