#include "../../../include/forest_ground_attachment.h"

void func_801627E8(void)
{
    ForestGroundAttachment *attachment = BFM_GROUND_ATTACHMENT_ROOT();
    if (attachment->step) {
        s32 index = attachment->step - 1;
        BFM_GROUND_ATTACHMENT_EFFECT(801896C0, index, attachment);
    }
}
