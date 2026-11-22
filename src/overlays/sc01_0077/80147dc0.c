/* Apply the current model's X, Y and Z rotations to the actor's local velocity. */
#include "forest_ground_orientation.h"
#include "forest_action.h"

extern void func_800484EC(void *matrix, void *input, void *output);

#if defined(__GNUC__) && __GNUC__ == 2 && defined(__mips__)
#define MOTION_REGISTER(n) __asm__(n)
#define MOTION_INPUTS(a, b) __asm__("" : : "r"(a), "r"(b))
#else
#define MOTION_REGISTER(n)
#define MOTION_INPUTS(a, b) ((void)0)
#endif

void func_80147DC0(ForestActionPlayer *actor, s32 *out)
{
    struct { u32 reserved[4]; u32 matrix[8]; } frame;
    register ForestActionPlayer *player MOTION_REGISTER("$18");
    register s32 *result MOTION_REGISTER("$19");
    register u8 *matrix MOTION_REGISTER("$17");
    register u8 *model MOTION_REGISTER("$16");

    player = actor;
    result = out;
    MOTION_INPUTS(player, result);
    matrix = (u8 *)frame.matrix;
    model = FOREST_ORIENTATION_BYTES(player->model, 0x16, FOREST_ORIENTATION_SUBOBJECT);
    FOREST_ORIENTATION_IDENTITY(matrix);
    FOREST_ORIENTATION_ROTATE(800126C4, matrix, BFM_GROUND_ATTACHMENT_HALF(model, 0x10));
    FOREST_ORIENTATION_ROTATE(80012558, matrix, BFM_GROUND_ATTACHMENT_HALF(model, 0x12));
    FOREST_ORIENTATION_ROTATE(800123F0, matrix, BFM_GROUND_ATTACHMENT_HALF(model, 0x14));
    func_800484EC(matrix, (u8 *)player + 0x24, result);
}
