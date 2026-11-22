#include "psx_types.h"

struct Obj8014A2E4;
extern struct Obj8014A2E4 *func_8014A2E4(void *obj);
extern void func_8014A380(char *obj, void *target);

s32 func_801508B4(void *obj)
{
    struct Obj8014A2E4 *target = func_8014A2E4(obj);
    if (target != 0) {
        func_8014A380(obj, target);
        return 1;
    }
    return 0;
}
