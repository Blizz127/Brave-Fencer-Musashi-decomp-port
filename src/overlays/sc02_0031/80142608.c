/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the sc01_0005 twin at this address (W18). */
/* Spawns the effect record for the object: from the plain allocator when the mode half-word at 0x70
 * is set, else from the allocator that takes the object's linked record (0x64). The new record is
 * stored at 0x20, initialised through func_8001CA1C and given half-width 0x200 fields; when the
 * object is not in plain mode the linked record's half-word at 0x36 is copied to 0xFE. A missing
 * record (or a missing link) runs the failure handler; otherwise the effect registers are cleared
 * and func_8012AD50 runs. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x18];
    s16 h18;      /* 0x18 */
    s16 h1A;      /* 0x1A */
    u8 pad1c[4];
    u32 w20;      /* 0x20 */
} Eff;

typedef struct {
    u8 pad0[0x36];
    u16 h36;      /* 0x36 */
} Src;

typedef struct {
    u8 pad0[0x20];
    u32 w20;      /* 0x20 */
    u8 pad24[0x40];
    u32 w64;      /* 0x64 */
    u8 pad68[8];
    s16 h70;      /* 0x70 */
    u8 pad72[0x1E];
    u32 w90;      /* 0x90 */
    u8 pad94[4];
    s16 h98;      /* 0x98 */
    u8 pad9a[0x62];
    s16 hFC;      /* 0xFC */
    s16 hFE;      /* 0xFE */
} Obj;

extern u8 D_80186088[];

extern u32 func_8012C1B8(void);
extern u32 func_8012C1DC(u32 src);
extern void func_8001CA1C(u32 e, u8 *tbl);
extern void func_8012CAE4(Obj *o);
extern void func_8012AD50(Obj *o);

void func_80142608(Obj *o)
{
    u32 e;
    u32 src;

    if (o->h70 != 0) {
        e = func_8012C1B8();
    } else {
        e = func_8012C1DC(o->w64);
    }
    o->w20 = e;
    if (e == 0) goto fail;
    func_8001CA1C(e, D_80186088);
    ((Eff *)e)->h1A = 0x200;
    ((Eff *)e)->h18 = 0x200;
    o->hFC = 0x40;
    if (o->h70 != 0) goto tail;
    src = o->w64;
    if (src == 0) goto fail;
    o->hFE = ((Src *)src)->h36;
    goto tail;
fail:
    func_8012CAE4(o);
    goto end;
tail:
    o->h98 = 0;
    o->w90 = 0;
    func_8012AD50(o);
end:
    return;
}
