/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef void (*Fn)(void);

extern void func_8012A598(Fn fp);

void func_8012A568(Fn fp) {
    func_8012A598(fp);
    fp();
}
