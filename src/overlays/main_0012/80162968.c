/* Orient the attached model using the owner's ground normal and height. */
#include "../../../include/forest_ground_orientation.h"

void func_80162968(ForestGroundAttachment *attachment)
{
    u8 *source = FOREST_ORIENTATION_BYTES(attachment->source, 0x86, FOREST_ORIENTATION_SOURCE);
    u8 *target = FOREST_ORIENTATION_BYTES(attachment->target, 0x1e, FOREST_ORIENTATION_TARGET);
    u8 *matrix = (u8 *)attachment + 0x0c;
    u8 *subobject;
    s32 z, y, magnitude;
    s16 scale;
    s16 height;

    FOREST_ORIENTATION_STORE(target, 0x10, FOREST_ORIENTATION_ANGLE(
        BFM_GROUND_ATTACHMENT_HALF(source, 0x84), BFM_GROUND_ATTACHMENT_HALF(source, 0x82)));
    z = BFM_GROUND_ATTACHMENT_HALF(source, 0x84);
    y = BFM_GROUND_ATTACHMENT_HALF(source, 0x82);
    magnitude = FOREST_ORIENTATION_MAGNITUDE((s32)((u32)(z * z) + (u32)(y * y)));
    FOREST_ORIENTATION_STORE(target, 0x14, FOREST_ORIENTATION_ANGLE(
        BFM_GROUND_ATTACHMENT_HALF(source, 0x80), -(s16)magnitude));
    FOREST_ORIENTATION_IDENTITY(matrix);
    FOREST_ORIENTATION_ROTATE(800126C4, matrix, BFM_GROUND_ATTACHMENT_HALF(target, 0x10));
    FOREST_ORIENTATION_ROTATE(800123F0, matrix, BFM_GROUND_ATTACHMENT_HALF(target, 0x14));
    subobject = FOREST_ORIENTATION_BYTES(BFM_GROUND_ATTACHMENT_READ(source, 0x20), 0x14, FOREST_ORIENTATION_SUBOBJECT);
    FOREST_ORIENTATION_ROTATE(80012558, matrix, BFM_GROUND_ATTACHMENT_HALF(subobject, 0x12));
    FOREST_ORIENTATION_ROTATE(800126C4, matrix, 0x400);
    height = (s16)((u32)attachment->y - (u16)BFM_GROUND_ATTACHMENT_HALF(source, 0x0a));
    scale = ((0x300 - height) * 0x2000) / 0x300;
    if (BFM_GROUND_ATTACHMENT_READ(source, 0x44) & 0x400)
        scale /= 3;
    FOREST_ORIENTATION_STORE(target, 0x1c, scale);
    FOREST_ORIENTATION_STORE(target, 0x1a, scale);
    FOREST_ORIENTATION_STORE(target, 0x18, scale);
}
