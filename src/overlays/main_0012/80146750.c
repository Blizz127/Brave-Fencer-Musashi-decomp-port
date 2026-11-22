#include "../../../include/forest_event.h"

ForestEventSlot *func_80146750(ForestActionEventRecord *request)
{
    s32 i;
    ForestEventSlot *slot;
    ForestEventSlot *result;
    i = 8;
    result = &D_8011F9D0[8];
    for (; i < 20; result++, i++) {
        slot = &D_8011F9D0[i];
        if (slot->kind == 0) {
            BFM_STORE_EVENT_FIELD(D_8011F9D0[i].actor, request, actor);
            BFM_STORE_EVENT_FIELD(slot->kind, request, type);
            BFM_STORE_EVENT_FIELD(D_8011F9D0[i].x, request, zero);
            BFM_STORE_EVENT_FIELD(D_8011F9D0[i].y, request, amount);
            BFM_STORE_EVENT_FIELD(D_8011F9D0[i].z, request, phase);
            BFM_STORE_EVENT_FIELD(D_8011F9D0[i].first, request, first);
            BFM_STORE_EVENT_FIELD(D_8011F9D0[i].second, request, second);
            return result;
        }
    }
    return 0;
}
