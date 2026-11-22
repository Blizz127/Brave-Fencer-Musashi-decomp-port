#include "../../../include/forest_ground_attachment.h"

s32 func_80162834(ForestGroundAttachment *attachment)
{
    return BFM_GROUND_ATTACHMENT_CALL(801896C4, attachment->kind, attachment);
}
