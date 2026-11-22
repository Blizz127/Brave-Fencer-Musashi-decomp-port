/* Initialize the field actor pool and its control state before placing room actors. */
#include "actor_position_updates.h"
#include "mine_initial_placement.h"

extern s16 D_801270C2;
extern u16 D_801270C4[];
extern u8 D_801270C6, D_801270C7;
extern s32 D_801274D8, D_801274DC, D_801274E0;
extern void func_80016714(s8 *destination, s32 size);

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
/* The original linker places the field work area eight bytes after the mode. */
#define FIELD_WORK(mode) ((s8 *)((u32)(mode) + 8))
#else
extern s8 D_801270C8[];
#define FIELD_WORK(mode) D_801270C8
#endif

void func_80178608(void)
{
    s16 *mode = &D_801270C0;

    func_80016714((s8 *)D_801202A0, 96 * sizeof(Obj8014A2E4));
    *mode = 1;
    D_801270C2 = 0;
    D_801270C4[0] = 1;
    D_801270C6 = 0;
    D_801270C7 = 0;
    D_801274D8 = 0x240000;
    D_801274DC = 0x271000;
    D_801274E0 = 0x400000;
    func_80016714(FIELD_WORK(mode), 1024);
    func_8013E4B4();
}
