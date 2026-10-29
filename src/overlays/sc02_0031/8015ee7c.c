/* Owner re-derivation from retail assembly (clean input set). */
/* Straight-line init sequence on o: func_8014CBF8, func_801474EC, func_80154274(o,&D_800D4C48),
 * func_80146DB8(o,&D_801872F0), func_801477E8(o, 0xFFF40000), func_80146CA0(o). */
#include "psx_types.h"

extern void func_8014CBF8(void *o);
extern void func_801474EC(void *o);
extern void func_80154274(void *o, void *a);
extern s32 D_800D4C48;
extern void func_80146DB8(void *o, void *m);
extern s32 D_801872F0;
extern void func_801477E8(u8 *o, s32 a);
extern void func_80146CA0(void *o);

void func_8015EE7C(u8 *o)
{
    func_8014CBF8(o);
    func_801474EC(o);
    func_80154274(o, &D_800D4C48);
    func_80146DB8(o, &D_801872F0);
    func_801477E8(o, (s32)0xFFF40000);
    func_80146CA0(o);
}
