/* The local-point record starts sixteen bytes before the current pose. */
#include "mine_pose_points.h"
#include "mine_model_sequence.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
extern u8 *D_80126B78;
#else
#include <string.h>
extern u8 D_80126B58[];
#endif

void func_801484B0(u8 *first, u8 *second)
{
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    const u8 *pose = *(const u8 **)(D_80126B78 + 0x20);
#else
    const u8 *model;
    MineSequenceWord position;
    memcpy(&model, D_80126B58 + 0x20, sizeof model);
    memcpy(&position, model + 0x20, sizeof position);
    const u8 *pose = (const u8 *)position;
#endif
    func_801485B8(pose - 16, first, second);
}
