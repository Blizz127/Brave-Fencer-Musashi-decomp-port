/* Owner re-derivation from retail assembly (clean input set). */
/* If effect 0x119 is active and func_8016F1AC allows, releases the handle stored at +0x198 and clears the effect. */
#include "psx_types.h"
int func_80029178(int id);
int func_8016F1AC(void);
void func_80139914(int h);
void func_80029124(int id, int v);
void func_8014C99C(char *o) {
    if ((u8)func_80029178(0x119) != 0) {
        if (func_8016F1AC() != 0) {
            func_80139914(*(int *)(o + 0x198));
            *(int *)(o + 0x198) = 0;
            func_80029124(0x119, 0);
        }
    }
}
