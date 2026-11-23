/* Commit the cemetery key award when its event script reaches this step. */
#include "psx_types.h"
#include "event_flags.h"

extern u32 func_800D0EC4(void);
extern void func_800D0F0C(u32 slot, u32 item);
extern void func_800291A0(int index, int value);
extern void func_80029514(int progress);
extern void func_80171928(void *script);

void func_8017D828(void *script)
{
    u32 slot = func_800D0EC4();

    func_800D0F0C(slot, 77);
    func_800291A0(204, 15);
    func_80029514(60);
    func_80029124(230, 1);
    func_80171928(script);
}
