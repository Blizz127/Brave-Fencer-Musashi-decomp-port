/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Fn)(Obj *);

struct Obj {
    u16 f0;       /* 0x0 */
};

extern void func_80149020(Obj *o);
extern Fn D_8018730C[];
extern void func_80147078(Obj *o, u16 a1);
extern void func_80159B70(Obj *o);

void func_80161034(Obj *o) {
    func_80149020(o);
    D_8018730C[o->f0](o);
    func_80147078(o, 0);
    func_80159B70(o);
}
