/* Owner re-derivation from retail assembly (clean input set). */
/* Queries input state via func_80133784 and combines it with the entry's type byte (+6) into a flag word; bit 0x1000 is set for types 2 and 0x1B. */
#include "psx_types.h"
int func_80133784(int a, void *b, void *c);
int func_8012CEB0(void *a0, char *e, int mode) {
    u16 buf[4];
    int r;
    int x;
    if (mode & 1) {
        int b;
        r = func_80133784((short)mode, a0, e);
        b = *(u8 *)(e + 6);
        r = r | b;
        if (b == 2 || b == 0x1B) {
            r |= 0x1000;
        }
        return r;
    }
    r = func_80133784(0, a0, e);
    if (r == 0) {
        return 0;
    }
    x = func_80133784(2, e, buf);
    if ((x & 0x6000) == 0) {
        return 0;
    }
    {
        int b;
        x &= 0xFFFF7FFF;
        x |= r & 0x8000;
        b = *(u8 *)(e + 6);
        *(u16 *)(e + 2) = buf[1];
        x |= b;
        if (b == 2 || b == 0x1B) {
            x |= 0x1000;
        }
        return x;
    }
}
