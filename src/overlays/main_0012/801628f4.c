#include "../../../include/forest_ground_attachment.h"

void func_801628F4(ForestGroundAttachment *attachment)
{
    u8 *source = BFM_GROUND_ATTACHMENT_SOURCE(attachment->source);
    u8 *target = BFM_GROUND_ATTACHMENT_TARGET(attachment->target);

    if (BFM_GROUND_ATTACHMENT_HALF(source, 0x7e))
        BFM_GROUND_ATTACHMENT_WRITE(target, 4, BFM_GROUND_ATTACHMENT_READ(target, 4) | 0x80000000u);
    else
        BFM_GROUND_ATTACHMENT_WRITE(target, 4, BFM_GROUND_ATTACHMENT_READ(target, 4) & 0x7fffffffu);
    attachment->x = BFM_GROUND_ATTACHMENT_HALF(source, 0x78);
    attachment->y = BFM_GROUND_ATTACHMENT_HALF(source, 0x7a);
    attachment->z = BFM_GROUND_ATTACHMENT_HALF(source, 0x7c);
    BFM_GROUND_ATTACHMENT_ORIENT(attachment);
}
